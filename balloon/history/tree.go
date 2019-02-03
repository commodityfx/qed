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

package history

import (
	"math/bits"

	"github.com/bbva/qed/metrics"

	"github.com/bbva/qed/balloon/cache"
	"github.com/bbva/qed/balloon/history/pruning"
	"github.com/bbva/qed/balloon/history/visit"
	"github.com/bbva/qed/hashing"
	"github.com/bbva/qed/log"
	"github.com/bbva/qed/storage"
)

type HistoryTree struct {
	hasherF    func() hashing.Hasher
	hasher     hashing.Hasher
	writeCache cache.ModifiableCache
	readCache  cache.Cache
}

func NewHistoryTree(hasherF func() hashing.Hasher, store storage.Store, cacheSize uint16) *HistoryTree {

	// create cache for Adding
	writeCache := cache.NewLruReadThroughCache(storage.HistoryCachePrefix, store, cacheSize)

	// create cache for Membership and Incremental
	readCache := cache.NewPassThroughCache(storage.HistoryCachePrefix, store)

	return &HistoryTree{
		hasherF:    hasherF,
		hasher:     hasherF(),
		writeCache: writeCache,
		readCache:  readCache,
	}
}

func (t *HistoryTree) getDepth(version uint64) uint16 {
	return uint16(bits.Len64(version))
}

func (t *HistoryTree) Add(eventDigest hashing.Digest, version uint64) (hashing.Digest, []*storage.Mutation, error) {

	// Activate metrics gathering
	stats := metrics.History

	// visitors
	computeHash := visit.NewComputeHashVisitor(t.hasher)
	caching := visit.NewCachingVisitor(computeHash, t.writeCache)
	collect := visit.NewCollectMutationsVisitor(caching, storage.HistoryCachePrefix)

	// build pruning context
	context := pruning.NewPruningContext(
		pruning.NewSingleTargetedCacheResolver(version),
		t.writeCache,
	)

	// traverse from root and generate a visitable pruned tree
	pruned, err := pruning.NewInsertPruner(version, eventDigest, context).Prune()
	if err != nil {
		return nil, nil, err
	}

	print := visit.NewPrintVisitor(t.getDepth(version))
	pruned.PreOrder(print)
	log.Debugf("Pruned tree: %s", print.Result())

	// visit the pruned tree
	rh := pruned.PostOrder(collect)

	// Increment add hits
	stats.Add("add_hits", 1)

	return rh, collect.Result(), nil
}

func (t *HistoryTree) ProveMembership(index, version uint64) (*MembershipProof, error) {

	log.Debugf("Proving membership for index %d with version %d", index, version)
	stats := metrics.History
	stats.Add("ProveMembership_hits", 1)
	// visitors
	computeHash := visit.NewComputeHashVisitor(t.hasherF())
	calcAuditPath := visit.NewAuditPathVisitor(computeHash)

	// build pruning context
	var resolver pruning.CacheResolver
	switch index == version {
	case true:
		resolver = pruning.NewSingleTargetedCacheResolver(version)
	case false:
		resolver = pruning.NewDoubleTargetedCacheResolver(index, version)
	}
	context := pruning.NewPruningContext(
		resolver,
		t.readCache,
	)

	// traverse from root and generate a visitable pruned tree
	pruned, err := pruning.NewSearchPruner(version, context).Prune()
	if err != nil {
		return nil, err
	}

	// print := NewPrintVisitor(t.getDepth(version))
	// pruned.PreOrder(print)
	// log.Debugf("Pruned tree: %s", print.Result())

	// visit the pruned tree
	pruned.PostOrder(calcAuditPath)

	proof := NewMembershipProof(index, version, calcAuditPath.Result(), t.hasherF())

	return proof, nil
}

func (t *HistoryTree) ProveConsistency(start, end uint64) (*IncrementalProof, error) {

	log.Debugf("Proving consistency between versions %d and %d", start, end)
	stats := metrics.History
	stats.Add("ProveConsistency_hits", 1)

	// visitors
	computeHash := visit.NewComputeHashVisitor(t.hasherF())
	calcAuditPath := visit.NewAuditPathVisitor(computeHash)

	// build pruning context
	context := pruning.NewPruningContext(
		pruning.NewIncrementalCacheResolver(start, end),
		t.readCache,
	)

	// traverse from root and generate a visitable pruned tree
	pruned, err := pruning.NewSearchPruner(end, context).Prune()
	if err != nil {
		return nil, err
	}

	// visit the pruned tree
	pruned.PostOrder(calcAuditPath)
	proof := NewIncrementalProof(start, end, calcAuditPath.Result(), t.hasherF())

	return proof, nil
}

func (t *HistoryTree) Close() {
	t.hasher = nil
	t.writeCache = nil
	t.readCache = nil
}
