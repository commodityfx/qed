package gossip

import (
	"net"

	"github.com/hashicorp/memberlist"
)

type NodeType int

func (t NodeType) String() string {
	switch t {
	case AuditorType:
		return "auditor"
	case MonitorType:
		return "monitor"
	case PublisherType:
		return "publisher"
	case ServerType:
		return "server"
	default:
		return "unknown"
	}
}

func NewNodeType(value string) NodeType {
	switch value {
	case "auditor":
		return AuditorType
	case "monitor":
		return MonitorType
	case "publisher":
		return PublisherType
	default:
		return ServerType
	}
}

const (
	AuditorType NodeType = iota
	MonitorType
	PublisherType
	ServerType
	MaxType
)

// This is the default port that we use for the Agent communication
const DefaultBindPort int = 7946

// DefaultConfig contains the defaults for configurations.
func DefaultConfig() *Config {
	return &Config{
		BindAddr:          "0.0.0.0",
		AdvertiseAddr:     "",
		LeaveOnTerm:       true,
		EnableCompression: false,
	}
}

// Config is the configuration for creating an Auditor instance
type Config struct {
	// The name of this node. This must be unique in the cluster. If this
	// is not set, Auditor will set it to the hostname of the running machine.
	NodeName string

	Role NodeType

	// BindAddr is the address that the Auditor agent's communication ports
	// will bind to. Auditor will use this address to bind to for both TCP
	// and UDP connections. If no port is present in the address, the default
	// port will be used.
	BindAddr string

	// AdvertiseAddr is the address that the Auditor agent will advertise to
	// other members of the cluster. Can be used for basic NAT traversal
	// where both the internal ip:port and external ip:port are known.
	AdvertiseAddr string

	// LeaveOnTerm controls if the Auditor does a graceful leave when receiving
	// the TERM signal. Defaults false. This can be changed on reload.
	LeaveOnTerm bool

	// EnableCompression specifies whether message compression is enabled
	// by `github.com/hashicorp/memberlist` when broadcasting events.
	EnableCompression bool

	// MemberlistConfig is the memberlist configuration that Aidotpr will
	// use to do the underlying membership management and gossip. Some
	// fields in the MemberlistConfig will be overwritten by Auditor no
	// matter what:
	//
	//   * Name - This will always be set to the same as the NodeName
	//     in this configuration.
	//
	//   * Events - Auditor uses a custom event delegate.
	//
	//   * Delegate - Auditor uses a custom delegate.
	//
	MemberlistConfig *memberlist.Config
}

// AddrParts returns the parts of the BindAddr that should be
// used to configure the Node.
func (c *Config) AddrParts(address string) (string, int, error) {
	_, _, err := net.SplitHostPort(address)
	if err != nil {
		return "", 0, err
	}

	addr, err := net.ResolveTCPAddr("tcp", address)
	if err != nil {
		return "", 0, err
	}

	return addr.IP.String(), addr.Port, nil
}
