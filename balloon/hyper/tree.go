/*
   Copyright 2018 Banco Bilbao Vizcaya Argentaria, S.A.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

package hyper

import (
	"bytes"
	"math"
	"sync"

	"github.com/bbva/qed/balloon/cache"
	"github.com/bbva/qed/balloon/navigator"
	"github.com/bbva/qed/balloon/visitor"
	"github.com/bbva/qed/hashing"
	"github.com/bbva/qed/log"
	"github.com/bbva/qed/storage"
	"github.com/bbva/qed/util"
)

type HyperTree struct {
	lock          sync.RWMutex
	store         storage.Store
	cache         cache.ModifiableCache
	hasherF       func() hashing.Hasher
	cacheLevel    uint16
	defaultHashes []hashing.Digest
	hasher        hashing.Hasher
}

func NewHyperTree(hasherF func() hashing.Hasher, store storage.Store, cache cache.ModifiableCache) *HyperTree {
	var lock sync.RWMutex
	hasher := hasherF()
	cacheLevel := hasher.Len() - uint16(math.Max(float64(2), math.Floor(float64(hasher.Len())/10)))
	tree := &HyperTree{
		lock:          lock,
		store:         store,
		cache:         cache,
		hasherF:       hasherF,
		cacheLevel:    cacheLevel,
		defaultHashes: make([]hashing.Digest, hasher.Len()),
		hasher:        hasher,
	}

	tree.defaultHashes[0] = tree.hasher.Do([]byte{0x0}, []byte{0x0})
	for i := uint16(1); i < hasher.Len(); i++ {
		tree.defaultHashes[i] = tree.hasher.Do(tree.defaultHashes[i-1], tree.defaultHashes[i-1])
	}

	// warm-up cache
	tree.RebuildCache()

	return tree
}

func (t *HyperTree) Add(eventDigest hashing.Digest, version uint64) (hashing.Digest, []*storage.Mutation, error) {
	t.lock.Lock()
	defer t.lock.Unlock()

	// visitors
	computeHash := visitor.NewComputeHashVisitor(t.hasher)
	caching := visitor.NewCachingVisitor(computeHash, t.cache)
	collect := visitor.NewCollectMutationsVisitor(caching, storage.HyperCachePrefix)

	// build pruning context
	versionAsBytes := util.Uint64AsBytes(version)
	context := PruningContext{
		navigator:     NewHyperTreeNavigator(t.hasher.Len()),
		cacheResolver: NewSingleTargetedCacheResolver(t.hasher.Len(), t.cacheLevel, eventDigest),
		cache:         t.cache,
		store:         t.store,
		defaultHashes: t.defaultHashes,
	}

	// traverse from root and generate a visitable pruned tree
	pruned, err := NewInsertPruner(eventDigest, versionAsBytes, context).Prune()
	if err != nil {
		return nil, nil, err
	}

	// visit the pruned tree
	rootHash := pruned.PostOrder(collect).(hashing.Digest)

	// create a mutation for the new leaf
	leafMutation := storage.NewMutation(storage.IndexPrefix, eventDigest, versionAsBytes)

	// collect mutations
	mutations := append(collect.Result(), leafMutation)

	return rootHash, mutations, nil
}

func (t *HyperTree) QueryMembership(eventDigest hashing.Digest) (proof *QueryProof, exists bool, err error) {
	t.lock.Lock()
	defer t.lock.Unlock()

	pair, err := t.store.Get(storage.IndexPrefix, eventDigest)
	if err == storage.ErrKeyNotFound {
		return nil, false, nil
	} else if err != nil {
		return nil, false, err
	}

	// visitors
	computeHash := visitor.NewComputeHashVisitor(t.hasher)
	calcAuditPath := visitor.NewAuditPathVisitor(computeHash)

	// build pruning context
	context := PruningContext{
		navigator:     NewHyperTreeNavigator(t.hasher.Len()),
		cacheResolver: NewSingleTargetedCacheResolver(t.hasher.Len(), t.cacheLevel, eventDigest),
		cache:         t.cache,
		store:         t.store,
		defaultHashes: t.defaultHashes,
	}

	// traverse from root and generate a visitable pruned tree
	pruned, err := NewSearchPruner(eventDigest, context).Prune()
	if err != nil {
		return nil, err
	}

	// visit the pruned tree
	pruned.PostOrder(calcAuditPath)

	return NewQueryProof(pair.Key, pair.Value, calcAuditPath.Result(), t.hasherF()), true, nil // TODO include version in audit path visitor
}

func (t *HyperTree) VerifyMembership(proof *QueryProof, version uint64, eventDigest, expectedDigest hashing.Digest) bool {
	t.lock.Lock()
	defer t.lock.Unlock()

	log.Debugf("Verifying membership for eventDigest %x", eventDigest)

	// visitors
	computeHash := visitor.NewComputeHashVisitor(t.hasher)

	// build pruning context
	versionAsBytes := util.Uint64AsBytes(version)
	context := PruningContext{
		navigator:     NewHyperTreeNavigator(t.hasher.Len()),
		cacheResolver: NewSingleTargetedCacheResolver(t.hasher.Len(), t.cacheLevel, eventDigest),
		cache:         proof.AuditPath(),
		store:         t.store,
		defaultHashes: t.defaultHashes,
	}

	// traverse from root and generate a visitable pruned tree
	pruned, err := NewVerifyPruner(eventDigest, versionAsBytes, context).Prune()
	if err != nil {
		return false
	}

	// visit the pruned tree
	recomputed := pruned.PostOrder(computeHash).(hashing.Digest)
	return bytes.Equal(recomputed, expectedDigest)
}

func (t *HyperTree) Close() {
	t.cache = nil
	t.hasher = nil
	t.defaultHashes = nil
	t.store = nil
}

func (t *HyperTree) RebuildCache() error {
	t.lock.Lock()
	defer t.lock.Unlock()

	// warm up cache
	log.Info("Warming up hyper cache...")

	// Fill last cache level with stored data
	err := t.cache.Fill(t.store.GetAll(storage.HyperCachePrefix))
	if err != nil {
		return err
	}

	if t.cache.Size() == 0 { // nothing to recompute
		log.Infof("Warming up done, elements cached: %d", t.cache.Size())
		return nil
	}

	// Recompute and fill the rest of the cache
	navigator := NewHyperTreeNavigator(t.hasher.Len())
	root := navigator.Root()
	// skip root
	t.populateCache(navigator.GoToLeft(root), navigator)
	t.populateCache(navigator.GoToRight(root), navigator)
	log.Infof("Warming up done, elements cached: %d", t.cache.Size())
	return nil
}

func (t *HyperTree) populateCache(pos navigator.Position, nav navigator.TreeNavigator) hashing.Digest {
	if pos.Height() == t.cacheLevel+1 {
		cached, ok := t.cache.Get(pos)
		if !ok {
			return nil
		}
		return cached
	}
	leftPos := nav.GoToLeft(pos)
	rightPos := nav.GoToRight(pos)
	left := t.populateCache(leftPos, nav)
	right := t.populateCache(rightPos, nav)

	if left == nil && right == nil {
		return nil
	}
	if left == nil {
		left = t.defaultHashes[leftPos.Height()]
	}
	if right == nil {
		right = t.defaultHashes[rightPos.Height()]
	}

	digest := t.hasher.Salted(pos.Bytes(), left, right)
	t.cache.Put(pos, digest)
	return digest
}
