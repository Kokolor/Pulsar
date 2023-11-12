TARGET = BOOTX64.EFI
include uefi/Makefile

disk:
	dd if=/dev/zero of=Pulsar.img bs=1M count=64
	mkfs.fat -F32 Pulsar.img
	mkdir mnt/
	sudo mount -o loop Pulsar.img mnt/
	sudo mkdir -p mnt/EFI/BOOT
	sudo cp $(TARGET) mnt/EFI/BOOT/
	sudo cp data/* mnt/EFI/BOOT
	sudo umount mnt/
	rm -rf mnt/

run: disk
	qemu-system-x86_64 -cpu qemu64,pdpe1gb -bios /usr/share/ovmf/OVMF.fd -drive file=Pulsar.img,format=raw -m 4096M

_clean:
	rm -f $(TARGET)
	rm -f Pulsar.img
