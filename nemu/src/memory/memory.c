#include "nemu.h"
#include "burst.h"

uint32_t dram_read(hwaddr_t addr, size_t len);
void dram_write(hwaddr_t addr, size_t len, uint32_t data);
int is_mmio(hwaddr_t addr);
uint32_t mmio_read(hwaddr_t addr, size_t len, int map_no);
void mmio_write(hwaddr_t addr, size_t len, uint32_t data, int map_no);

lnaddr_t seg_translate(swaddr_t addr, size_t len, uint8_t sreg_id) {
	if(!cpu.cr0.protect_enable) return addr;
	Assert(sreg_id < 6, "invalid segment register %u", sreg_id);
	Assert(len > 0 && addr <= cpu.sreg[sreg_id].limit &&
		len - 1 <= cpu.sreg[sreg_id].limit - addr,
		"segment limit exceeded: sreg=%u addr=0x%x len=%u limit=0x%x",
		sreg_id, addr, (unsigned)len, cpu.sreg[sreg_id].limit);
	return cpu.sreg[sreg_id].base + addr;
}

static bool page_walk(lnaddr_t addr, hwaddr_t *result) {
	if(!(cpu.cr0.protect_enable && cpu.cr0.paging)) {
		*result = addr;
		return true;
	}

	uint32_t offset = addr & 0xfff;
	int hit = read_tlb(addr);
	if(hit >= 0) {
		*result = (tlb[hit].page_num << 12) | offset;
		return true;
	}

	hwaddr_t pde_addr = (cpu.cr3.page_directory_base << 12) |
		((addr >> 22) << 2);
	PageEntry pde;
	pde.val = hwaddr_read(pde_addr, 4);
	if(!pde.present) return false;

	hwaddr_t pte_addr = (pde.frame << 12) |
		(((addr >> 12) & 0x3ff) << 2);
	PageEntry pte;
	pte.val = hwaddr_read(pte_addr, 4);
	if(!pte.present) return false;

	*result = (pte.frame << 12) | offset;
	write_tlb(addr, *result);
	return true;
}

hwaddr_t page_translate(lnaddr_t addr) {
	hwaddr_t result;
	Assert(page_walk(addr, &result), "linear address 0x%08x is not mapped", addr);
	return result;
}

hwaddr_t page_translate_additional(lnaddr_t addr, int *flag) {
	*flag = 0;
	if(!(cpu.cr0.protect_enable && cpu.cr0.paging)) return addr;

	PageEntry pde;
	hwaddr_t pde_addr = (cpu.cr3.page_directory_base << 12) |
		((addr >> 22) << 2);
	pde.val = hwaddr_read(pde_addr, 4);
	if(!pde.present) { *flag = 1; return 0; }

	PageEntry pte;
	hwaddr_t pte_addr = (pde.frame << 12) |
		(((addr >> 12) & 0x3ff) << 2);
	pte.val = hwaddr_read(pte_addr, 4);
	if(!pte.present) { *flag = 2; return 0; }
	return (pte.frame << 12) | (addr & 0xfff);
}

uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	Assert(len >= 1 && len <= 4, "invalid physical read length %u", (unsigned)len);
	int map_no = is_mmio(addr);
	if(map_no >= 0) return mmio_read(addr, len, map_no);
	int first_line = read_cache1(addr);
	uint32_t offset = addr & (CACHE_L1_BLOCK_SIZE - 1);
	uint8_t bytes[4] = {0};
	if(offset + len <= CACHE_L1_BLOCK_SIZE) {
		memcpy(bytes, cache1[first_line].data + offset, len);
	} else {
		size_t first_len = CACHE_L1_BLOCK_SIZE - offset;
		int second_line = read_cache1(addr + first_len);
		memcpy(bytes, cache1[first_line].data + offset, first_len);
		memcpy(bytes + first_len, cache1[second_line].data, len - first_len);
	}
	uint32_t value = 0;
	memcpy(&value, bytes, len);
	return value;
}

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data) {
	Assert(len >= 1 && len <= 4, "invalid physical write length %u", (unsigned)len);
	int map_no = is_mmio(addr);
	if(map_no >= 0) {
		mmio_write(addr, len, data, map_no);
		return;
	}
	write_cache1(addr, len, data);
}

uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	Assert(len == 1 || len == 2 || len == 4, "invalid linear read length %u", (unsigned)len);
	uint32_t value = 0;
	size_t done = 0;
	while(done < len) {
		size_t page_left = 0x1000 - ((addr + done) & 0xfff);
		size_t chunk = len - done < page_left ? len - done : page_left;
		value |= hwaddr_read(page_translate(addr + done), chunk) << (done * 8);
		done += chunk;
	}
	return value;
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	Assert(len == 1 || len == 2 || len == 4, "invalid linear write length %u", (unsigned)len);
	size_t done = 0;
	while(done < len) {
		size_t page_left = 0x1000 - ((addr + done) & 0xfff);
		size_t chunk = len - done < page_left ? len - done : page_left;
		hwaddr_write(page_translate(addr + done), chunk, data >> (done * 8));
		done += chunk;
	}
}

uint32_t swaddr_read(swaddr_t addr, size_t len) {
	return lnaddr_read(seg_translate(addr, len, current_sreg), len);
}

void swaddr_write(swaddr_t addr, size_t len, uint32_t data) {
	lnaddr_write(seg_translate(addr, len, current_sreg), len, data);
}
