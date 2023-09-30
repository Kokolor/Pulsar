TARGET = BOOTX64.EFI
include uefi/Makefile

disk:
	dd if=/dev/zero of=TahaBoot.img bs=1M count=64
	mkfs.fat -F32 TahaBoot.img
	mkdir mnt/
	sudo mount -o loop TahaBoot.img mnt/
	sudo mkdir -p mnt/EFI/BOOT
	sudo cp $(TARGET) mnt/EFI/BOOT/
	sudo cp data/* mnt/EFI/BOOT
	sudo umount mnt/
	rm -rf mnt/

run: disk
	qemu-system-x86_64 -cpu qemu64,pdpe1gb -bios /usr/share/edk2/x64/OVMF.fd -drive file=TahaBoot.img,format=raw -m 4096M

_clean:
	rm -f $(TARGET)
	rm -f TahaBoot.img
