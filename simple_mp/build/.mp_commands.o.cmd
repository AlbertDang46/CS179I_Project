cmd_mp_commands.o = gcc -Wp,-MD,./.mp_commands.o.d.tmp  -m64 -pthread -I/users/Sinzunza/openNetVM/dpdk/lib/librte_eal/linux/include  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_RDSEED -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C -DRTE_MACHINE_CPUFLAG_AVX2  -I/users/Sinzunza/openNetVM/dpdk/examples/multi_process/simple_mp/build/include -DRTE_USE_FUNCTION_VERSIONING -I/users/Sinzunza/openNetVM/dpdk/x86_64-native-linuxapp-gcc/include -include /users/Sinzunza/openNetVM/dpdk/x86_64-native-linuxapp-gcc/include/rte_config.h -D_GNU_SOURCE -O3 -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Wdeprecated -Werror -Wno-missing-field-initializers -Wimplicit-fallthrough=2 -Wno-format-truncation -Wno-address-of-packed-member -DALLOW_EXPERIMENTAL_API    -o mp_commands.o -c /users/Sinzunza/openNetVM/dpdk/examples/multi_process/simple_mp/mp_commands.c 
