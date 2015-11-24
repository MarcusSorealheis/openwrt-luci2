# openwrt-luci2

## Usage
Your feeds.conf.default (or feeds.conf) should contain a line like:
```
src-git luci2 https://github.com/liudf0716/openwrt-luci2.git
```

To install all its package definitions, run:
```
./scripts/feeds update luci2
./scripts/feeds install -a -p luci2
```
