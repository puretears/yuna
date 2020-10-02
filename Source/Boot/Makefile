output_dir=Output
bins=$(output_dir)/boot.bin $(output_dir)/loader.bin

all: $(bins)

$(output_dir)/boot.bin: boot.asm
	nasm $< -o $@

$(output_dir)/loader.bin: loader.asm
	nasm $< -o $@

.PHONY: all clean

clean:
	- rm $(output_dir)/*