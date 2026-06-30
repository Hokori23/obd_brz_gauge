# `firmware`

这里放的是预编译固件，不是源码模块。

当你暂时不想搭 ESP-IDF 环境，只想先把设备刷起来验证效果时，可以直接使用这里的二进制文件。

## 文件

- `release/bootloader/bootloader.bin`
- `release/partition_table/partition-table.bin`
- `release/obd_brz_gauge.bin`

## 烧录地址

- `0x0` -> `bootloader/bootloader.bin`
- `0x8000` -> `partition_table/partition-table.bin`
- `0x10000` -> `obd_brz_gauge.bin`

## 示例命令

```bash
esptool.py --chip esp32s3 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 bootloader/bootloader.bin 0x8000 partition_table/partition-table.bin 0x10000 obd_brz_gauge.bin
```

## 学习建议

你现在的目标是理解源码，所以这里知道用途即可，不需要先深挖。
