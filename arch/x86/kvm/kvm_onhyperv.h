/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * KVM L1 hypervisor optimizations on Hyper-V.
 */

#ifndef __ARCH_X86_KVM_KVM_ONHYPERV_H__
#define __ARCH_X86_KVM_KVM_ONHYPERV_H__

#if IS_ENABLED(CONFIG_HYPERV)

enum tdp_pointers_status {
	TDP_POINTERS_CHECK = 0,
	TDP_POINTERS_MATCH = 1,
	TDP_POINTERS_MISMATCH = 2
};

static inline void kvm_update_arch_tdp_pointer(struct kvm_vcpu *vcpu,
		u64 tdp_pointer)
{
	struct kvm *kvm = vcpu->kvm;

	spin_lock(&kvm->arch.tdp_pointer_lock);
	vcpu->arch.tdp_pointer = tdp_pointer;
	kvm->arch.tdp_pointers_match = TDP_POINTERS_CHECK;
	spin_unlock(&kvm->arch.tdp_pointer_lock);
}

int kvm_hv_remote_flush_tlb(struct kvm *kvm);
int kvm_hv_remote_flush_tlb_with_range(struct kvm *kvm,
		struct kvm_tlb_range *range);
#endif
#endif

