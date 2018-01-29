# input for generate_ninja_targets.pl

# do not instrument normaliz code for gathering test coverage
( $ConfigFlags{'bundled.libnormaliz.UseBundled'}
  ? ( 'libnormaliz_inst.cc' => { CmodeFLAGS => '${CexternModeFLAGS}' } )
  : ( IGNORE => { 'libnormaliz_inst.cc' => 1 } )
)
