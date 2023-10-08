# TODO: run cmake if build-PICO does not exist
ELF_FILE=build-PICO/firmware.elf
if [ -f "$ELF_FILE" ]; then
	echo === Removing $ELF_FILE
	rm $ELF_FILE
fi
echo === Running make
make
echo === Done.
# TODO: native beep
jpo.cmd beep