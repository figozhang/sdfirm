#!/bin/sh

SCRIPT=`(cd \`dirname $0\`; pwd)`
GEM5_SRC=~/workspace/gem5
GEM5_DBG=Minor
GEM5_IMG=linaro-minimal-aarch64.img

while getopts "imrs:ux" opt
do
	case $opt in
	i) GEM5_DBG=${GEM5_DBG},Faults,GIC;;
	m) GEM5_DBG=${GEM5_DBG},MMU,TLB,AddrRanges;;
	r) GEM5_DBG=${GEM5_DBG},Registers,-IntRegs,-CCRegs;;
	x) GEM5_DBG=${GEM5_DBG},Exec;;
	s) GEM5_SRC=$OPTARG;;
	u) GEM5_IMG=aarch64-ubuntu-trusty-headless.img;;
	?) echo "Invalid argument $opt"
	   fatal_usage;;
	esac
done
shift $(($OPTIND - 1))

SDFIRM_SRC=`(cd ${SCRIPT}/..; pwd)`
GEM5_SRC=`(cd ${GEM5_SRC}; pwd)`

cp ${SDFIRM_SRC}/sdfirm.strip ${GEM5_SRC}/fs_images/arm/binaries/boot_emm.arm64

(
	cd ${GEM5_SRC}
	export M5_PATH="${GEM5_SRC}/fs_images/arm/"

	./build/ARM/gem5.opt --debug-flags=${GEM5_DBG} \
		configs/example/fs.py \
		--kernel=vmlinux.aarch64.20140821 \
		--machine-type=VExpress_EMM64 \
		--dtb-file=vexpress.aarch64.20140821.dtb \
		--disk-image=${GEM5_IMG}
	cat ${GEM5_SRC}/m5out/system.terminal
)
