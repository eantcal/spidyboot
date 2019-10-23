# spidyboot

## SPI bootable image maker
This is a tool to create or modify SPI bootable images for Freescale PowerPC spi/flash devices.

 The spidyboot utility can take the following flags and arguments:
```
   --help | --ver  | --show 
   --bin <src_binary_file> --cfg <cfg_file> --dat <dat_file> 
 [ --prb <preamble_file> ] 
 [ --spi -s <bootcode_file> -d <spiboot_file> | --patch <spiboot_file> ] 
 [ --addr <baddr> <newaddr> ]
 [ --tga <trgaddr> ] [ --sra <srcaddr> ] [ --exe <exeaddr> ] 
```

Use
 - "--help" to show help message.
 - "--ver" to show program version.
 - "--show" to print the preamble content. 
 - "--bin <src_binary_file>" to read the preamble from a binary file (<src_binary_file>).

 - "--cfg <cfg_file>" to modify the preamble by using data read from a DRAM config file.

You may use "Component DDR for QorIQ - DDR Controller Configuration Utility"  provided by Freescale (R) NetComm utility to generate a memory initialization file (.cfg).  

An example of generated ddrCtrl_1.cfg file is the following one:
 
 ```
  ##################################################
  # ProcessorExpert DDR Tool memory initialization #
  ##################################################

   # DDR Controller 1 Registers

   # DDR_SDRAM_CFG
   writemem.l 0xFE008110 0x670C0000

   # CS0_BNDS
   writemem.l 0xFE008000 0x3F

   # CS0_CONFIG
   writemem.l 0xFE008080 0x80014302

   # CS0_CONFIG_2
   writemem.l 0xFE0080C0 0x00

   # TIMING_CFG_3
   writemem.l 0xFE008100 0x01031000

   # TIMING_CFG_0
   writemem.l 0xFE008104 0x00330104

   # TIMING_CFG_1
   writemem.l 0xFE008108 0x98912733

   # TIMING_CFG_2
   writemem.l 0xFE00810C 0x010860D3

   # DDR_SDRAM_CFG_2
   writemem.l 0xFE008114 0x24000012

   # DDR_SDRAM_MODE
   writemem.l 0xFE008118 0x40061650

   # DDR_SDRAM_MODE_2
   writemem.l 0xFE00811C 0x8040C000

   # DDR_SDRAM_MD_CNTL
   writemem.l 0xFE008120 0x00000000

   # DDR_SDRAM_INTERVAL
   writemem.l 0xFE008124 0x2DAE0100

   # DDR_DATA_INIT
   writemem.l 0xFE008128 0xDEADBEEF

   # DDR_SDRAM_CLK_CNTL
   writemem.l 0xFE008130 0x00800000

   # DDR_INIT_ADDR
   writemem.l 0xFE008148 0x00000000

   # DDR_INIT_EXT_ADDR
   writemem.l 0xFE00814C 0x00000000

   # TIMING_CFG_4
   writemem.l 0xFE008160 0x01

   # TIMING_CFG_5
   writemem.l 0xFE008164 0x3400

   # DDR_ZQ_CNTL
   writemem.l 0xFE008170 0x89080600

   # DDR_WRLVL_CNTL
   writemem.l 0xFE008174 0x85049400

   # DDR_SR_CNTR
   writemem.l 0xFE00817C 0x00000000

   # DDR_WRLVL_CNTL_2
   writemem.l 0xFE008190   0x00000000

   # DDR_WRLVL_CNTL_3
   writemem.l 0xFE008194   0x00000000

   # DDRCDR_1
   writemem.l 0xFE008B28 0x00000000

   # DDRCDR_2
   writemem.l 0xFE008B2C 0x00000000

   # ERR_SBE
   writemem.l 0xFE008E58 0x00010000

   #delay before enable
   sleep 500
   #DDR_SDRAM_CFG
   writemem.l 0xFE008110 0xE70C0000

   # wait for DRAM data initialization
   sleep 1000

```

- --dat <dat_file> to modify the preamble by using data read from a DAT file.
DAT file format is a list of pairs with syntax <offset>:<value> where <offset> is the position in the preamble, and <value> is the corresponding value that will be patched.

An example of .dat file is following one:

```
  040:424f4f54
  044:00000000
  048:00080000
  04c:00000000
  050:00001000
  054:00000000
  058:11000000
  05c:00000000
  060:1107f000
  064:00000000
  068:00000017

  080:ff702110
  084:470c0008
  088:ff702000
  08c:0000003f
  090:ff702080
  094:80014302
  098:ff702100
  09c:00030000
  0a0:ff702104
  0a4:00110104
  0a8:ff702108
  0ac:6f6b8644
  0b0:ff70210c
  0b4:0fa888cf
  0b8:ff702114
  0bc:24401000
  0c0:ff702118
  0c4:00441420
  0c8:ff702124
```

Some .dat files are provided with "MentorEmbedded / boot-format" tool. See [https://github.com/MentorEmbedded/boot-format](https://github.com/MentorEmbedded/boot-format) to obtain more information



- "--prb <preamble_file>" to save the preamble in the file <preamble_file>.
- "--spi -s <bootcode_file> -d <spiboot_file>" to create a spi-flash image: 
```<spiboot_file> = preamble + <bootcode_file>.```

- "--patch <spiboot_file>" to patch the preamble of an existing spi-flash image.
- "--addr <baddr> <newaddr>" to replace the base address <baddr> with new value <newaddr>.
- "--tga <trgaddr>" to replace the default target address with new value <trgaddr>.
- "--sra <srcaddr>" to replace the default source address with new value <srcaddr>.
- "--exe <exeaddr>" to replace the default exe start address with new value <exeaddr>.

##Examples.

- To print a default preamble type:
```
         $ ./spidyboot --show

         0x40- 0x43 BOOT signature      :  0x42 0x4f 0x4f 0x54 == 'BOOT' OK
         0x48- 0x4B User's code length  :  0x00080000 (524288 bytes - 512 Kb)
         0x50- 0x53 Source Address      :  0x00000400
         0x58- 0x5B Target Address      :  0x11000000
         0x60- 0x63 Exe Start Address   :  0x1107f000
         0x68- 0x6B N.of Adr/Data pairs :  0x00000000 (0)
```
 
- To write a default preamble into a binary file and show it you may use the following command:

```
         $ ./spidyboot --show --prb preamble.bin

         0x40- 0x43 BOOT signature      :  0x42 0x4f 0x4f 0x54 == 'BOOT' OK
         0x48- 0x4B User's code length  :  0x00080000 (524288 bytes - 512 Kb)
         0x50- 0x53 Source Address      :  0x00000400
         0x58- 0x5B Target Address      :  0x11000000
         0x60- 0x63 Exe Start Address   :  0x1107f000
         0x68- 0x6B N.of Adr/Data pairs :  0x00000000 (0)
```

- To generate a spi-flash boot image by using information of .cfg file (ddrCtrl_1.cfg) and an image of u-boot (u-boot.bin) you may use the following command:
```
         $ ./spidyboot --spi -s u-boot.bin -d spi_u-boot.bin --cfg ddrCtrl_1.cfg
```

- To generate a spi-flash boot image by using information of .dat file (ddr2cfg.dat) and an image of u-boot (u-boot.bin) you may use the following command:
```
         $ ./spidyboot --spi -s u-boot.bin -d spi_u-boot.bin --dat ddr2cfg.dat
```

Appending --show parameter to the last commands you can also dump the preamble content:

```
         $ ./spidyboot --spi -s u-boot.bin -d spi_u-boot.bin --cfg ddrCtrl_1.dat --show

          0x40- 0x43 BOOT signature      :  0x42 0x4f 0x4f 0x54 == 'BOOT' OK
          0x48- 0x4B User's code length  :  0x00080000 (524288 bytes - 512 Kb)
          0x50- 0x53 Source Address      :  0x00000400
          0x58- 0x5B Target Address      :  0x11000000
          0x60- 0x63 Exe Start Address   :  0x1107f000
          0x68- 0x6B N.of Adr/Data pairs :  0x00000016 (22)
          0x080-0x087 addr[ 0]@0xff702110 := 0x470c0008
          0x088-0x08f addr[ 1]@0xff702000 := 0x0000003f
          0x090-0x097 addr[ 2]@0xff702080 := 0x80014302
          0x098-0x09f addr[ 3]@0xff702100 := 0x00030000
          0x0a0-0x0a7 addr[ 4]@0xff702104 := 0x00110104
          0x0a8-0x0af addr[ 5]@0xff702108 := 0x6f6b8644
          0x0b0-0x0b7 addr[ 6]@0xff70210c := 0x0fa888cf
          0x0b8-0x0bf addr[ 7]@0xff702114 := 0x24401000
          0x0c0-0x0c7 addr[ 8]@0xff702118 := 0x00441420
          0x0c8-0x0cf addr[ 9]@0xff702124 := 0x0c300100
          0x0d0-0x0d7 addr[10]@0xff702128 := 0xdeadbeef
          0x0d8-0x0df addr[11]@0xff702130 := 0x03000000
          0x0e0-0x0e7 addr[12]@0xff702160 := 0x00000001
          0x0e8-0x0ef addr[13]@0xff702164 := 0x02401400
          0x0f0-0x0f7 addr[14]@0xff702170 := 0x89080600
          0x0f8-0x0ff addr[15]@0xff702174 := 0x8675f608
          0x100-0x107 addr[16]@0xff702084 := 0x00000000
          0x108-0x10f addr[17]@0x40000001 := 0x00000100
          0x110-0x117 addr[18]@0xff702110 := 0xc70c0008
          0x118-0x11f addr[19]@0xff700c08 := 0x00000000
          0x120-0x127 addr[20]@0xff700c10 := 0x80f0001d
          0x128-0x12f addr[21]@0xefefefef := 0x00000000
```
If you want modify an existing spi-flash boot image you can use --patch paramter instead of --spi. You may replace all the preamble content or modify one of source address, target address, exe start address.

You may also replace the base address used in the assignment list. All the command line parameters may be combined in order to do all such things.
