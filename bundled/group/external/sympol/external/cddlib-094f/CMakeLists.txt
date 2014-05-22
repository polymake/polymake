add_definitions(-DPACKAGE=\"cddlib\" -DVERSION=\"0.94\" -DHAVE_LIBGMP=1 -DSTDC_HEADERS=1 -DGMPRATIONAL)
add_library(cddgmp SHARED cddmp.c cddproj.c cddlp.c cddlib.c cddio.c cddcore.c setoper.c 
 cddmp_f.c cddproj_f.c cddlp_f.c cddlib_f.c cddcore_f.c cddio_f.c)

target_link_libraries(cddgmp ${GMP_LIBRARIES})

install (TARGETS cddgmp DESTINATION lib)
