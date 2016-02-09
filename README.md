# Manage your LightNVM Solid State Drives

Tool to adminstrate and configure your drives.

### Examples
- General information from LightNVM kernel module
  - ```lnvm info```
- List available devices
  - ```lnvm devices```
- Init target (tgt0) with (nvme0n1) device using rrpc on lun 0.
  - ``` lnvm create -d nvme0n1 -n tgt0 -t rrpc```
- Init target (test0) with (nulln0) device using rrpc on luns [0->3].
  - ```lnvm create -d nulln0 -n test0 -t rrpc -o 0:3```
- Remove target (tgt0).
  - ```lnvm remove tgt0```
  - ```lnvm remove -n tgt0```

### How to use

```
Usage: ladm [OPTION...] [<cmd> [CMD-OPTIONS]]

Supported commands are:
  devices      List available LightNVM devices.
  info         List general info and target engines
  create       Create target on top of a specific device
  remove       Remove target from device

  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version
```

#### Create

```
Usage: ladm create [OPTION...]

  -d, --device=DEVICE        LightNVM device e.g. nvme0n1
  -n, --tgtname=TGTNAME      Target name e.g. tgt0
  -o, --tgtoptions=          Options e.g. 0:0
  -t, --tgttype=TGTTYPE      Target type e.g. rrpc

Examples:
 Init target (tgt0) with (nvme0n1) device using rrpc on lun 0.
  lnvm create -d nvme0n1 -n tgt0 -t rrpc
 Init target (test0) with (nulln0) device using rrpc on luns [0->3].
  lnvm create -d nulln0 -n test0 -t rrpc -o 0:3
```

#### Remove
```
Usage: ladm remove [OPTION...] [TGTNAME|-n TGTNAME]

  -n, --tgtname=TGTNAME      Target name e.g. tgt0

Example:
 Remove target (tgt0).
  lnvm remove tgt0
  lnvm remove -n tgt0
```
### Compile

```
  git clone https://github.com/OpenChannelSSD/lightnvm-adm.git
  cd lightnvm-adm
  make
```
