# Manage your LightNVM Solid State Drives

Tool to manage and configure your drives.

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
Usage: lnvm [OPTION...] [<cmd> [CMD-OPTIONS]]

Supported commands are:
  init         Initialize device for LightNVM
  devices      List available LightNVM devices.
  info         List general info and target engines
  create       Create target on top of a specific device
  remove       Remove target from device
  factory      Reset device to factory state

  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version
```

#### Init

```
Usage: lnvm init [OPTION...]


  -d, --device=DEVICE        LightNVM device e.g. nvme0n1
  -m, --mmname=MMNAME        Media Manager. Default: gennvm
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Examples:
 Init nvme0n1 device
  lnvm init -d nvme0n1
 Init nvme0n1 device with other media manager
  lnvm init -d nvne0n1 -m other
```

#### Create

```
Usage: lnvm create [OPTION...]


  -d, --device=DEVICE        LightNVM device e.g. nvme0n1
  -n, --tgtname=TGTNAME      Target name e.g. tgt0
  -o, --tgtoptions=          Options e.g. 0:0
  -t, --tgttype=TGTTYPE      Target type e.g. rrpc
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Examples:
 Init target (tgt0) with (nvme0n1) device using rrpc on lun 0.
  lnvm create -d nvme0n1 -n tgt0 -t rrpc
 Init target (test0) with (nulln0) device using rrpc on luns [0->3].
  lnvm create -d nulln0 -n test0 -t rrpc -o 0:3
```

#### Remove

```
Usage: lnvm remove [OPTION...] [TGTNAME|-n TGTNAME]


  -n, --tgtname=TGTNAME      Target name e.g. tgt0
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Example:
 Remove target (tgt0).
  lnvm remove tgt0
  lnvm remove -n tgt0
```

### Compile

```
  git clone https://github.com/OpenChannelSSD/lnvm.git
  cd lightnvm-adm
  make
```
