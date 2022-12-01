set(CMAKE_CXX_COMPILER "/nix/store/x8rwpqd4np9gkgazcz65zrf3k3zi6frl-gcc-wrapper-12.2.0/bin/g++")
set(CMAKE_CXX_COMPILER_ARG1 "")
set(CMAKE_CXX_COMPILER_ID "GNU")
set(CMAKE_CXX_COMPILER_VERSION "12.2.0")
set(CMAKE_CXX_COMPILER_VERSION_INTERNAL "")
set(CMAKE_CXX_COMPILER_WRAPPER "")
set(CMAKE_CXX_STANDARD_COMPUTED_DEFAULT "17")
set(CMAKE_CXX_EXTENSIONS_COMPUTED_DEFAULT "ON")
set(CMAKE_CXX_COMPILE_FEATURES "cxx_std_98;cxx_template_template_parameters;cxx_std_11;cxx_alias_templates;cxx_alignas;cxx_alignof;cxx_attributes;cxx_auto_type;cxx_constexpr;cxx_decltype;cxx_decltype_incomplete_return_types;cxx_default_function_template_args;cxx_defaulted_functions;cxx_defaulted_move_initializers;cxx_delegating_constructors;cxx_deleted_functions;cxx_enum_forward_declarations;cxx_explicit_conversions;cxx_extended_friend_declarations;cxx_extern_templates;cxx_final;cxx_func_identifier;cxx_generalized_initializers;cxx_inheriting_constructors;cxx_inline_namespaces;cxx_lambdas;cxx_local_type_template_args;cxx_long_long_type;cxx_noexcept;cxx_nonstatic_member_init;cxx_nullptr;cxx_override;cxx_range_for;cxx_raw_string_literals;cxx_reference_qualified_functions;cxx_right_angle_brackets;cxx_rvalue_references;cxx_sizeof_member;cxx_static_assert;cxx_strong_enums;cxx_thread_local;cxx_trailing_return_types;cxx_unicode_literals;cxx_uniform_initialization;cxx_unrestricted_unions;cxx_user_literals;cxx_variadic_macros;cxx_variadic_templates;cxx_std_14;cxx_aggregate_default_initializers;cxx_attribute_deprecated;cxx_binary_literals;cxx_contextual_conversions;cxx_decltype_auto;cxx_digit_separators;cxx_generic_lambdas;cxx_lambda_init_captures;cxx_relaxed_constexpr;cxx_return_type_deduction;cxx_variable_templates;cxx_std_17;cxx_std_20;cxx_std_23")
set(CMAKE_CXX98_COMPILE_FEATURES "cxx_std_98;cxx_template_template_parameters")
set(CMAKE_CXX11_COMPILE_FEATURES "cxx_std_11;cxx_alias_templates;cxx_alignas;cxx_alignof;cxx_attributes;cxx_auto_type;cxx_constexpr;cxx_decltype;cxx_decltype_incomplete_return_types;cxx_default_function_template_args;cxx_defaulted_functions;cxx_defaulted_move_initializers;cxx_delegating_constructors;cxx_deleted_functions;cxx_enum_forward_declarations;cxx_explicit_conversions;cxx_extended_friend_declarations;cxx_extern_templates;cxx_final;cxx_func_identifier;cxx_generalized_initializers;cxx_inheriting_constructors;cxx_inline_namespaces;cxx_lambdas;cxx_local_type_template_args;cxx_long_long_type;cxx_noexcept;cxx_nonstatic_member_init;cxx_nullptr;cxx_override;cxx_range_for;cxx_raw_string_literals;cxx_reference_qualified_functions;cxx_right_angle_brackets;cxx_rvalue_references;cxx_sizeof_member;cxx_static_assert;cxx_strong_enums;cxx_thread_local;cxx_trailing_return_types;cxx_unicode_literals;cxx_uniform_initialization;cxx_unrestricted_unions;cxx_user_literals;cxx_variadic_macros;cxx_variadic_templates")
set(CMAKE_CXX14_COMPILE_FEATURES "cxx_std_14;cxx_aggregate_default_initializers;cxx_attribute_deprecated;cxx_binary_literals;cxx_contextual_conversions;cxx_decltype_auto;cxx_digit_separators;cxx_generic_lambdas;cxx_lambda_init_captures;cxx_relaxed_constexpr;cxx_return_type_deduction;cxx_variable_templates")
set(CMAKE_CXX17_COMPILE_FEATURES "cxx_std_17")
set(CMAKE_CXX20_COMPILE_FEATURES "cxx_std_20")
set(CMAKE_CXX23_COMPILE_FEATURES "cxx_std_23")

set(CMAKE_CXX_PLATFORM_ID "Linux")
set(CMAKE_CXX_SIMULATE_ID "")
set(CMAKE_CXX_COMPILER_FRONTEND_VARIANT "")
set(CMAKE_CXX_SIMULATE_VERSION "")




set(CMAKE_AR "/nix/store/x8rwpqd4np9gkgazcz65zrf3k3zi6frl-gcc-wrapper-12.2.0/bin/ar")
set(CMAKE_CXX_COMPILER_AR "/nix/store/74siignawpdycmrvisilm5hwgqjry7zm-gcc-12.2.0/bin/gcc-ar")
set(CMAKE_RANLIB "/nix/store/x8rwpqd4np9gkgazcz65zrf3k3zi6frl-gcc-wrapper-12.2.0/bin/ranlib")
set(CMAKE_CXX_COMPILER_RANLIB "/nix/store/74siignawpdycmrvisilm5hwgqjry7zm-gcc-12.2.0/bin/gcc-ranlib")
set(CMAKE_LINKER "/nix/store/x8rwpqd4np9gkgazcz65zrf3k3zi6frl-gcc-wrapper-12.2.0/bin/ld")
set(CMAKE_MT "")
set(CMAKE_COMPILER_IS_GNUCXX 1)
set(CMAKE_CXX_COMPILER_LOADED 1)
set(CMAKE_CXX_COMPILER_WORKS TRUE)
set(CMAKE_CXX_ABI_COMPILED TRUE)

set(CMAKE_CXX_COMPILER_ENV_VAR "CXX")

set(CMAKE_CXX_COMPILER_ID_RUN 1)
set(CMAKE_CXX_SOURCE_FILE_EXTENSIONS C;M;c++;cc;cpp;cxx;m;mm;mpp;CPP;ixx;cppm)
set(CMAKE_CXX_IGNORE_EXTENSIONS inl;h;hpp;HPP;H;o;O;obj;OBJ;def;DEF;rc;RC)

foreach (lang C OBJC OBJCXX)
  if (CMAKE_${lang}_COMPILER_ID_RUN)
    foreach(extension IN LISTS CMAKE_${lang}_SOURCE_FILE_EXTENSIONS)
      list(REMOVE_ITEM CMAKE_CXX_SOURCE_FILE_EXTENSIONS ${extension})
    endforeach()
  endif()
endforeach()

set(CMAKE_CXX_LINKER_PREFERENCE 30)
set(CMAKE_CXX_LINKER_PREFERENCE_PROPAGATES 1)

# Save compiler ABI information.
set(CMAKE_CXX_SIZEOF_DATA_PTR "8")
set(CMAKE_CXX_COMPILER_ABI "ELF")
set(CMAKE_CXX_BYTE_ORDER "LITTLE_ENDIAN")
set(CMAKE_CXX_LIBRARY_ARCHITECTURE "")

if(CMAKE_CXX_SIZEOF_DATA_PTR)
  set(CMAKE_SIZEOF_VOID_P "${CMAKE_CXX_SIZEOF_DATA_PTR}")
endif()

if(CMAKE_CXX_COMPILER_ABI)
  set(CMAKE_INTERNAL_PLATFORM_ABI "${CMAKE_CXX_COMPILER_ABI}")
endif()

if(CMAKE_CXX_LIBRARY_ARCHITECTURE)
  set(CMAKE_LIBRARY_ARCHITECTURE "")
endif()

set(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX "")
if(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX)
  set(CMAKE_CL_SHOWINCLUDES_PREFIX "${CMAKE_CXX_CL_SHOWINCLUDES_PREFIX}")
endif()





set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "/nix/store/qc8rlhdcdxaf6dwbvv0v4k50w937fyzj-python3-3.10.8/include;/nix/store/f95kxwhnr2bazy7nl6wzwjiak02dlp9v-openssl-3.0.7-dev/include;/nix/store/ll72ba8dm16jxp1qhm77kv8s782lwz9m-gsl-2.7.1/include;/nix/store/xf0ssp8s6xjz710q33hspj5dphqhmmc1-libxcrypt-4.4.30/include;/nix/store/dll25930s2zds9l5r9cp5c20mb1wngrg-valgrind-3.19.0-dev/include;/nix/store/ljxng913nd56l0g4f4syp64xxzx2dzg3-llvm-14.0.6-dev/include;/nix/store/66l0ncimnsjpg791an7bbh6z3k8sxvn5-ncurses-6.3-p20220507-dev/include;/nix/store/0y971q54v6jm9ss243xhl4y0gnlsm9c8-zlib-1.2.13-dev/include;/nix/store/cgy1nn58zc2mjzaqvv8xvs8akfbzqra3-gnuradio-wrapped-3.10.4.0/include;/nix/store/x45h8x6b992w06bln0g56q4180ggdicl-mpir-3.0.0/include;/nix/store/2f5qpav0z1k3b5a7p529kixq0xm2dvp5-volk-2.5.0/include;/nix/store/h406hban9hz2fqi5py2155jr5ijpki01-boost-1.79.0-dev/include;/nix/store/piw78nk18rgmygmngxs598b6jpbjm7c7-spdlog-1.10.0-dev/include;/nix/store/2irvs11cr55ngkibwwh0lm15dxq0j6lc-fmt-8.1.1-dev/include;/nix/store/kzcd1ggki8477sp090znwnh6k86vqkik-python3-3.10.8-env/include;/nix/store/fy11na5n20fpllg9mkfgkv94km57f81d-gmp-with-cxx-6.2.1-dev/include;/nix/store/k60q9b1zajvh0xxcn9v319401sqzy2ic-libsndfile-1.1.0-dev/include;/nix/store/h84pdy3g0cy45vxy9pg9iqyn4ksiq9fx-fftw-single-3.3.10-dev/include;/nix/store/4c7g0zcz8rkyk0s11xa67xvab45a9g7s-libX11-1.8.1-dev/include;/nix/store/4wglnplxrmkkazsrpshya8dy37mcsc6f-xorgproto-2021.5/include;/nix/store/aa2zl46fmxrj54y96whvhw4x7iappl06-libxcb-1.14-dev/include;/nix/store/r1gp89sb3rxjsvyw67l293q0s2a3k1c6-libXrandr-1.5.2-dev/include;/nix/store/igx3jc4zgy47cd5d6iygwil4g7jri57v-libXrender-0.9.10-dev/include;/nix/store/2p1fj1g020ji904hj169aa594ak7nijz-libXinerama-1.1.4-dev/include;/nix/store/5ad6lk12knm0wrqk3zqwdzj1ypqw09v7-libXcursor-1.2.0-dev/include;/nix/store/r3rx2y2krmhwa5wwxmc4jhlfqgbp1wxc-libXi-1.8-dev/include;/nix/store/gy9xfsa2krcv3zfjcqp8c7xn2ydb9q5a-libXfixes-6.0.0-dev/include;/nix/store/zbzn3myvn5cjll3i400wq69s85a5g09n-libXext-1.3.4-dev/include;/nix/store/kadfaq2cbm1ksd3b62xhaaxai8kxjg8z-libXau-1.0.9-dev/include;/nix/store/idj0nrdr3wpmc5jvib2gyip1hlp9al0s-glfw-3.3.8/include;/nix/store/q7rykhz2b21dzchh25yfyj4dd1ddf701-libGL-1.5.0-dev/include;/nix/store/74siignawpdycmrvisilm5hwgqjry7zm-gcc-12.2.0/include/c++/12.2.0;/nix/store/74siignawpdycmrvisilm5hwgqjry7zm-gcc-12.2.0/include/c++/12.2.0/x86_64-unknown-linux-gnu;/nix/store/74siignawpdycmrvisilm5hwgqjry7zm-gcc-12.2.0/include/c++/12.2.0/backward;/nix/store/74siignawpdycmrvisilm5hwgqjry7zm-gcc-12.2.0/lib/gcc/x86_64-unknown-linux-gnu/12.2.0/include;/nix/store/74siignawpdycmrvisilm5hwgqjry7zm-gcc-12.2.0/include;/nix/store/74siignawpdycmrvisilm5hwgqjry7zm-gcc-12.2.0/lib/gcc/x86_64-unknown-linux-gnu/12.2.0/include-fixed;/nix/store/4pqv2mwdn88h7xvsm7a5zplrd8sxzvw0-glibc-2.35-163-dev/include")
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "stdc++;m;gcc_s;gcc;c;gcc_s;gcc")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "/nix/store/qc8rlhdcdxaf6dwbvv0v4k50w937fyzj-python3-3.10.8/lib;/nix/store/4mxnw95jcm5a27qk60z7yc0gvxp42b9a-openssl-3.0.7/lib;/nix/store/ll72ba8dm16jxp1qhm77kv8s782lwz9m-gsl-2.7.1/lib;/nix/store/xf0ssp8s6xjz710q33hspj5dphqhmmc1-libxcrypt-4.4.30/lib;/nix/store/1i5ah27gxx3a3fyjyydfwwzqq8ni33i8-ncurses-6.3-p20220507/lib;/nix/store/026hln0aq1hyshaxsdvhg0kmcm6yf45r-zlib-1.2.13/lib;/nix/store/d1mbq7i1sq4hjgs4j5p346fwn8ap2l8v-llvm-14.0.6-lib/lib;/nix/store/cgy1nn58zc2mjzaqvv8xvs8akfbzqra3-gnuradio-wrapped-3.10.4.0/lib;/nix/store/x45h8x6b992w06bln0g56q4180ggdicl-mpir-3.0.0/lib;/nix/store/2f5qpav0z1k3b5a7p529kixq0xm2dvp5-volk-2.5.0/lib;/nix/store/lf8ghiy89rfhqqyh9j98270afwjqzyrb-boost-1.79.0/lib;/nix/store/ypr1hzw3yzza98jpjwjpf53cdg1k095f-fmt-8.1.1/lib;/nix/store/707sf98v81xb07pqn5z6h6hbshgdq95b-spdlog-1.10.0/lib;/nix/store/kzcd1ggki8477sp090znwnh6k86vqkik-python3-3.10.8-env/lib;/nix/store/ijz81p08bp812q2bvv77lz9qpfzncibd-gmp-with-cxx-6.2.1/lib;/nix/store/zqz61kfyb3vhxqkg1ihrvzmhgl5x18fc-libsndfile-1.1.0/lib;/nix/store/lgx9ccic6miz2wrbjhyw6gzbz12rf0pj-fftw-single-3.3.10/lib;/nix/store/wnfz9vlswaivvspywp58f0fwzngs18hd-libxcb-1.14/lib;/nix/store/j45y03xv9x733p6ksnixhvsj0ysscjjl-libX11-1.8.1/lib;/nix/store/6m0886ras7ckvn16g1mri2v8606v47i8-libXrender-0.9.10/lib;/nix/store/wd8slvf94jry0i61x8k56akjfnh7l9cw-libXrandr-1.5.2/lib;/nix/store/ki85i1r11jqwhq0x0bhx2nh7d3zvz7sd-libXinerama-1.1.4/lib;/nix/store/zviv012sk17psw90xcj9a5m5cvj5d75x-libXcursor-1.2.0/lib;/nix/store/i0nklcxb0w2y7r057mna2k4madqja9qi-libXfixes-6.0.0/lib;/nix/store/43zqsx4aylp1jpxlbszw744dch4hyn8i-libXau-1.0.9/lib;/nix/store/gx73knn2rn3bqbna0c88fxw8k6g107n1-libXext-1.3.4/lib;/nix/store/62an9fayrsyqg48mx5cdrlcbbv1c29di-libXi-1.8/lib;/nix/store/idj0nrdr3wpmc5jvib2gyip1hlp9al0s-glfw-3.3.8/lib;/nix/store/bzjrdzawqd0s45w0ilynvlviyvgmjsjs-libGL-1.5.0/lib;/nix/store/p2y84gb1srlfqvyvkapp5hrak9fax0h8-libglvnd-1.5.0/lib;/nix/store/4nlgxhb09sdr51nc9hdm8az5b08vzkgx-glibc-2.35-163/lib;/nix/store/pvgwppp14mfqg01a605vb0hbkgy4i164-gcc-12.2.0-lib/lib;/nix/store/x8rwpqd4np9gkgazcz65zrf3k3zi6frl-gcc-wrapper-12.2.0/bin;/nix/store/74siignawpdycmrvisilm5hwgqjry7zm-gcc-12.2.0/lib/gcc/x86_64-unknown-linux-gnu/12.2.0;/nix/store/74siignawpdycmrvisilm5hwgqjry7zm-gcc-12.2.0/lib64;/nix/store/74siignawpdycmrvisilm5hwgqjry7zm-gcc-12.2.0/lib")
set(CMAKE_CXX_IMPLICIT_LINK_FRAMEWORK_DIRECTORIES "")
