is_dbgr=0
is_fast=0

if [ $# -eq 0 ]; then
    echo "Both FAST and DEBUG builds"
	is_dbgr=1
	is_fast=1
elif [ "$1" == "fast" ]; then
    echo "FAST build only"
	is_fast=1
elif [ "$1" == "dbgr" ]; then
    echo "DBGR build only"
	is_dbgr=1
elif [ "$1" == "clean" ]; then
    echo "Removing build dirs"
	rm -rf "build-jpo-dbgr"
	rm -rf "build-jpo-fast"
	exit 0
else
    echo "Invalid argument. Values: dbgr|fast|clean."
	exit 1
fi

if [ $is_dbgr == 1 ]; then
	echo
	echo ==================
	echo === DBGR build ===
	echo ==================
	if [ ! -d "build-jpo-dbgr" ]; then
		echo "=== Directory build-jpo-dbgr does not exist, running cmake"
		cmake -G "Unix Makefiles" -S . -B build-jpo-dbgr -DPICO_BUILD_DOCS=0 \
			  -DMICROPY_BOARD=PICO -DMICROPY_BOARD_DIR=boards/PICO
	fi

	cd build-jpo-dbgr
	ELF_FILE=firmware.elf
	if [ -f "$ELF_FILE" ]; then
		echo Removing $ELF_FILE
		rm $ELF_FILE
	fi
	echo === Running make
	make
	cd ..
fi

if [ $is_fast == 1 ]; then
	echo
	echo ==================
	echo === FAST build ===
	echo ==================
	if [ ! -d "build-jpo-fast" ]; then
		echo "=== Directory build-jpo-fast does not exist, running cmake"
		cmake -G "Unix Makefiles" -S . -B build-jpo-fast -DPICO_BUILD_DOCS=0 \
			  -DMICROPY_BOARD=PICO -DMICROPY_BOARD_DIR=boards/PICO
	fi

	cd build-jpo-fast
	ELF_FILE=firmware.elf
	if [ -f "$ELF_FILE" ]; then
		echo Removing $ELF_FILE
		rm $ELF_FILE
	fi
	echo === Running make
	make
	cd ..
fi

echo === Done.
# TODO: native beep
jpo.cmd beep

