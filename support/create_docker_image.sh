#!/bin/bash

# args: prebuiltDir installDestDir basisImage moreLabels imageName
set -ex

[ $# = 5 -a -d "$1" -a -d "$2" ]
prebuiltDir="$1"
installDestDir="$2"
basisImage="$3"
moreLabels="$4"
imageName="$5"

# stuff jni libraries for MacOS, because the image is supposed to be run on Macs too
install -m 555 bundled/jreality/external/jreality/jni/macosx/* $installDestDir/usr/local/share/polymake/resources/java/jni/jreality/
# TODO: enable this when docker on Mac supports shared memory exchange between containers and host
# install -m 555 $prebuiltDir/libpolymake_java.jnilib $installDestDir/usr/local/share/polymake/resources/java/jni/

cid=$(docker create $basisImage)

docker cp $installDestDir/usr/local/. ${cid}:/usr/local/
docker commit -c 'LABEL '$(sed -n 's/^declare \$Version="\(.*\)";/polymake.version=\1/p' perllib/Polymake.pm)" polymake.git_revision=$(git rev-parse --short HEAD) $moreLabels" \
              -c 'ENTRYPOINT ["polymake"]' \
              -c 'ENV POLYMAKE_CONFIG_PATH /usr/local/lib/polymake/user_config;user' \
              -c 'ENV PATH /usr/local/share/polymake/resources/host-agent/bin:/opt/bin:/usr/local/bin:/usr/bin:/bin' \
              -c 'ENV POLYMAKE_BUILD_ROOT docker' \
              $cid polymake/$imageName
docker image prune -f
docker rm -v $cid

rm -rf $installDestDir/usr/local
