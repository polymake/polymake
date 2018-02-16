#!/bin/bash

# this script starts polymake in a docker container

: ${POLYMAKE_USER_DIR:=$HOME/.polymake}
declare run_docker=docker
declare -a mount_list env_list

# read in the configuration, provide the defaults if it does not exist yet
if [ ! -f $POLYMAKE_USER_DIR/container.config ]; then

if [ $# = 1 -a "$1" = "--uninstall" ]; then exit; fi

echo "Looks like it's the first time you are using polymake with docker"
echo "Try to check your access rights to docker..."

if ! docker --version ; then
  echo "docker seems to be completely missing, can't proceed"
  exit 1
fi

how_run_docker='run_docker="sudo docker"'

if [ "$(uname -s)" = Darwin ] || groups | grep -q '\<docker\>' && docker ps -q >/dev/null; then
  echo "You are in the docker group, the default setup should do"
  how_run_docker="## $how_run_docker"
else
  echo "You are not in the docker group, trying with sudo..."
  if ! sudo docker ps -q >/dev/null; then
    echo "docker cannot be used directly nor via sudo"
    echo "Please ask your system administrator for giving you sufficient access rights."
    exit 1
  fi
fi

mkdir -p $POLYMAKE_USER_DIR

cat <<EOF >$POLYMAKE_USER_DIR/container.config
# this is the user's custom configuration for the script polymake-in-container.sh

# public image for the container
declare -r image_name=polymake/nightly
declare -r image_tag=latest

# after how many days should a new image be pulled from DockerHub
# replace with 0 to disable automatic pulling altogether
declare -r pull_cadence=7

# assume the current user identity
declare -r userid=\$(id -u)
declare -r groupid=\$(id -g)

# size of temporary in-memory file system for the container
# change this only if some programs started by polymake complain about lacking space there
# polymake itself will place all temporary files in \$POLYMAKE_USER_DIR/tmp
declare -r tmpsize=250m

# add file systems to be mounted within the container
# your HOME, POLYMAKE_USER_DIR, and the current working directory are always mounted,
# don't put them here
## mount_list+=(-v <PATH>:<PATH>)

# add environment variables tp be passed to the container
# HOME, USER, and POLYMAKE_USER_DIR are always passed, don't list them here
## env_list+=(-e <VARNAME>=<VALUE>)

# uncomment this if you want to run docker using sudo
# by default, the script assumes that you belong to the group docker or other group
# entitled to run docker directly.
$how_run_docker

EOF
fi

. $POLYMAKE_USER_DIR/container.config

declare -r POLYMAKE_RESOURCE_DIR=$POLYMAKE_USER_DIR/resources
declare -r POLYMAKE_TEMP_DIR=$POLYMAKE_USER_DIR/tmp

if [ $# = 1 -a "$1" = "--uninstall" ]; then
  images=$(docker images --format '{{.ID}}' 'polymake/*')
  for iid in $images; do
    containers=$(docker ps -af ancestor=$iid --format={{.ID}})
    for cid in $containers; do
      docker rm -v $cid
    done
    docker rmi $iid
  done
  docker volume prune -f
  docker image prune -f
  rm -rf $POLYMAKE_RESOURCE_DIR $POLYMAKE_TEMP_DIR
  exit
fi

# always mount HOME and the current directory under identical paths in the container

mount_list+=(-v "${HOME}:${HOME}")
env_list+=(-e "HOME=$HOME" -e "USER=$USER" -e "LC_ALL=C.UTF-8")

case "$POLYMAKE_USER_DIR" in $HOME/*)
  ;;
*)
  mount_list+=(-v "${POLYMAKE_USER_DIR}:${POLYMAKE_USER_DIR}")
  ;;
esac
env_list+=(-e "POLYMAKE_USER_DIR=$POLYMAKE_USER_DIR")
env_list+=(-e "POLYMAKE_RESOURCE_DIR=$POLYMAKE_RESOURCE_DIR")

declare -r serversocket=$POLYMAKE_RESOURCE_DIR/host-agent/.socket

env_list+=(-e POLYMAKE_HOST_AGENT=$serversocket)
env_list+=(-e TMPDIR=$POLYMAKE_TEMP_DIR)

case "$PWD" in $HOME/*|$POLYMAKE_USER_DIR/*)
  ;;
*)
  mount_list+=(-v "${PWD}:${PWD}")
  ;;
esac

declare -r runopts="--rm=true --read-only --tmpfs /tmp:rw,exec,nosuid,size=$tmpsize --tmpfs /run:rw,noexec,nosuid,size=8m --ipc=host"

# establish enough ports for InteractiveViewer and children
# unfortunately, docker does not support dynamic allocation of mapped ports yet
declare -r portmapping="-p 127.0.0.1::30000-30010"

rm -rf $POLYMAKE_TEMP_DIR
mkdir -p $POLYMAKE_RESOURCE_DIR $POLYMAKE_TEMP_DIR

declare -r serverfile=$POLYMAKE_RESOURCE_DIR/host-agent/server.pl

find_newer=""
if [ -n "$pull_cadence" -a "$pull_cadence" != 0 ]; then
  find_newer="-mtime -$pull_cadence"
fi
declare -r serverfile_ok=$(find $POLYMAKE_RESOURCE_DIR -name server.pl $find_newer)

declare new_images="n"
declare copy_resources="n"

function pull_image() {
  $run_docker pull "$1" | perl -p -e 'BEGIN { $s=-1; $|=1; } if (/Status: Image is up to date/) { $s=1; } elsif (/Status: Downloaded newer image/) { $s=0; } END { exit($s); }'
}

declare -r thirdparty_name=polymake/thirdparty

if [ "$serverfile_ok" != $serverfile ]; then
  if pull_image ${image_name}:${image_tag}; then
    # retrieved newer image
    new_images="y"
    copy_resources="y"
    rm -rf $POLYMAKE_RESOURCE_DIR/*
  elif [ -f $serverfile ]; then
    # make the next pull attempt in $pull_cadence days, not in the next run
    touch $serverfile
  else
    copy_resources="y"
  fi
fi

declare -r thirdparty_tag=$($run_docker inspect -f '{{(index .Config.Labels "thirdparty.tag")}}' $($run_docker image ls -q ${image_name}:${image_tag}))

if [ $new_images = "y" ]; then
  if pull_image ${thirdparty_name}:${thirdparty_tag}; then
    old_vol=$($run_docker ps -af 'label=polymake.thirdparty-vol' --format={{.Names}})
    if [ -n "$old_vol" ]; then
      $run_docker rm -v $old_vol
    fi
  fi
  $run_docker image prune -f
fi

thirdparty_vol=$($run_docker ps -af ancestor=${thirdparty_name}:${thirdparty_tag} --format={{.Names}})
if [ -z "${thirdparty_vol}" ]; then
  echo "Starting container with third-party software volume..."
  thirdparty_vol=thirdparty-vol-${thirdparty_tag}
  $run_docker create --name $thirdparty_vol --label polymake.thirdparty-vol=true ${thirdparty_name}:${thirdparty_tag}
fi

declare -r cid=$($run_docker create -ti -u ${userid}:${groupid} $runopts "${mount_list[@]}" "${env_list[@]}" -w "$PWD" \
                             --volumes-from=${thirdparty_vol}:ro $portmapping \
                             ${image_name}:${image_tag} "$@")

if [ $copy_resources = "y" ]; then
  $run_docker cp ${cid}:/usr/local/share/polymake/resources/. $POLYMAKE_RESOURCE_DIR/
  case "$run_docker" in sudo*)
    sudo chown -R ${userid}:${groupid} $POLYMAKE_RESOURCE_DIR
    ;;
  esac
  if [ "${image_name}:${image_tag}" = "polymake/nightly:latest" -a \
       $POLYMAKE_RESOURCE_DIR/polymake-in-container.sh -nt $0 ] && \
     ! cmp -s $POLYMAKE_RESOURCE_DIR/polymake-in-container.sh $0; then
    if [ -w $0 ]; then
      echo "replacing $0 with a newer version"
      # can't overwrite the existing file because bash is still reading from it
      rm $0
      cp -p $POLYMAKE_RESOURCE_DIR/polymake-in-container.sh $0
    else
      echo "WARNING:"
      echo "A newer version of script polymake-in-container.sh is available at $POLYMAKE_RESOURCE_DIR/polymake-in-container.sh"
      echo "However, $0 is write-protected, can't update it automatically."
    fi
  fi
fi

exec perl $serverfile --socket $serversocket --docker-cmd "$run_docker" --cid $cid
