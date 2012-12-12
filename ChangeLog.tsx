2012-12-11  H.J. Lu  <hongjiu.lu@intel.com>

	* sysdeps/x86_64/multiarch/init-arch.c (__init_cpu_features):
	Initialize COMMON_CPUID_INDEX_7 element.
	* sysdeps/x86_64/multiarch/init-arch.h (bit_RTM): New macro.
	(index_RTM): Likewise.
	(CPUID_RTM): Likewise.
	(HAS_RTM): Likewise.
	(COMMON_CPUID_INDEX_7): New enum.
