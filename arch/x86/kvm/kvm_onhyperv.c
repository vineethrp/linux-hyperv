// SPDX-License-Identifier: GPL-2.0-only
/*
 * KVM L1 hypervisor optimizations on Hyper-V.
 */

#include <linux/kvm_host.h>
#include <asm/mshyperv.h>

#include "hyperv.h"
#include "kvm_onhyperv.h"

/* check_tdp_pointer() should be under protection of tdp_pointer_lock. */
static void check_tdp_pointer_match(struct kvm *kvm)
{
	u64 tdp_pointer = INVALID_PAGE;
	bool valid_tdp = false;
	struct kvm_vcpu *vcpu;
	int i;

	kvm_for_each_vcpu(i, vcpu, kvm) {
		if (!valid_tdp) {
			tdp_pointer = vcpu->arch.tdp_pointer;
			valid_tdp = true;
			continue;
		}

		if (tdp_pointer != vcpu->arch.tdp_pointer) {
			kvm->arch.tdp_pointers_match = TDP_POINTERS_MISMATCH;
			return;
		}
	}

	kvm->arch.tdp_pointers_match = TDP_POINTERS_MATCH;
}

static int kvm_fill_hv_flush_list_func(struct hv_guest_mapping_flush_list *flush,
		void *data)
{
	struct kvm_tlb_range *range = data;

	return hyperv_fill_flush_guest_mapping_list(flush, range->start_gfn,
			range->pages);
}

static inline int __hv_remote_flush_tlb_with_range(struct kvm *kvm,
		struct kvm_vcpu *vcpu, struct kvm_tlb_range *range)
{
	u64 tdp_pointer = vcpu->arch.tdp_pointer;

	/*
	 * FLUSH_GUEST_PHYSICAL_ADDRESS_SPACE hypercall needs address
	 * of the base of EPT PML4 table, strip off EPT configuration
	 * information.
	 */
	if (range)
		return hyperv_flush_guest_mapping_range(tdp_pointer & PAGE_MASK,
				kvm_fill_hv_flush_list_func, (void *)range);
	else
		return hyperv_flush_guest_mapping(tdp_pointer & PAGE_MASK);
}

int kvm_hv_remote_flush_tlb_with_range(struct kvm *kvm,
		struct kvm_tlb_range *range)
{
	struct kvm_vcpu *vcpu;
	int ret = 0, i;

	spin_lock(&kvm->arch.tdp_pointer_lock);

	if (kvm->arch.tdp_pointers_match == TDP_POINTERS_CHECK)
		check_tdp_pointer_match(kvm);

	if (kvm->arch.tdp_pointers_match != TDP_POINTERS_MATCH) {
		kvm_for_each_vcpu(i, vcpu, kvm) {
			/* If tdp_pointer is invalid pointer, bypass flush request. */
			if (VALID_PAGE(vcpu->arch.tdp_pointer))
				ret |= __hv_remote_flush_tlb_with_range(
					kvm, vcpu, range);
		}
	} else {
		ret = __hv_remote_flush_tlb_with_range(kvm,
				kvm_get_vcpu(kvm, 0), range);
	}

	spin_unlock(&kvm->arch.tdp_pointer_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(kvm_hv_remote_flush_tlb_with_range);

int kvm_hv_remote_flush_tlb(struct kvm *kvm)
{
	return kvm_hv_remote_flush_tlb_with_range(kvm, NULL);
}
EXPORT_SYMBOL_GPL(kvm_hv_remote_flush_tlb);
