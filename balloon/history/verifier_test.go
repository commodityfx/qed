package history

import (
	"bytes"
	"testing"
	"verifiabledata/balloon/hashing"
)

func TestVerify(t *testing.T) {

	var testCases = []struct {
		key                []byte
		auditPath          []Node
		version            uint
		expectedCommitment []byte
	}{
		// INCREMENTAL
		{
			key:                []byte{0x0},
			auditPath:          []Node{},
			version:            0,
			expectedCommitment: []byte{0x0},
		},
		{
			key: []byte{0x1},
			auditPath: []Node{
				Node{[]byte{0x0}, 0, 0},
			},
			version:            1,
			expectedCommitment: []byte{0x0},
		},
		{
			key: []byte{0x2},
			auditPath: []Node{
				Node{[]byte{0x1}, 0, 1},
			},
			version:            2,
			expectedCommitment: []byte{0x3},
		},
		{
			key: []byte{0x3},
			auditPath: []Node{
				Node{[]byte{0x2}, 2, 0},
				Node{[]byte{0x1}, 0, 1},
			},
			version:            3,
			expectedCommitment: []byte{0x0},
		},
		{
			key: []byte{0x4},
			auditPath: []Node{
				Node{[]byte{0x1}, 0, 2},
			},
			version:            4,
			expectedCommitment: []byte{0x4},
		},

		// LONGER VERSION
		{
			key: []byte{0x0},
			auditPath: []Node{
				Node{[]byte{0x1}, 1, 0},
				Node{[]byte{0x0}, 2, 1},
				Node{[]byte{0x4}, 4, 2},
			},
			version:            4,
			expectedCommitment: []byte{0x4},
		},
		{
			key: []byte{0x2},
			auditPath: []Node{
				Node{[]byte{0x0}, 0, 1},
				Node{[]byte{0x3}, 3, 0},
				Node{[]byte{0x4}, 4, 2},
			},
			version:            4,
			expectedCommitment: []byte{0x4},
		},
	}

	hasher := hashing.XorHasher
	verifier := NewVerifier(FakeLeafHasherF(hasher), FakeInteriorHasherF(hasher))

	for _, c := range testCases {
		correct, recomputed := verifier.Verify(c.expectedCommitment, c.auditPath, c.key, c.version)

		if !correct {
			t.Fatalf("The verification failed")
		}

		if bytes.Compare(recomputed, c.expectedCommitment) != 0 {
			t.Fatalf("Expected: %x, Actual: %x", c.expectedCommitment, recomputed)
		}
	}
}
