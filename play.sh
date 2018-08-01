#!/bin/sh

# -----BEGIN EASILY CONFIGURABLE OPTIONS-------
bin='typhoon'
opts='--cpus 4 --hash 512m --logfile play.log --egtbpath /egtb/three;/egtb/four;/egtb/five'
dir='.'
ics='chessclub.com'
flags=''
# -----END EASILY CONFIGURABLE OPTIONS-------

usage() {
  echo "Usage: $1 [--local | --icc | --fics] [--xdebug] [--edebug]"
  echo ""
  echo "--local and --icc toggle between playing locally against the engine and"
  echo "playing the engine on an ICS server.  Playing locally is the default."
  echo ""
  echo "--xdebug enables debugging of the xboard protocol.  --edebug uses a debug"
  echo "version of the chess engine."
  echo ""
  exit 1
}

play_locally() {
  xboard -ponder -fcp "${bin} ${opts}" -fd ${dir} -xpopup -xexit -size medium \
         -coords -highlight
}

attach_to_ics() {
  while true; do
    xboard -ics -icshost "${ics}" -ponder -fcp "${bin} ${opts}" -fd ${dir} \
           -autoflag -xpopup -xexit -size medium -coords -highlight -zp \
           ${flags}
    sleep 30
    killall typhoon
  done
}

mode='local'
for _switch ; do
  case $_switch in
    --xdebug)
      flags='-debug'
      shift
      ;;
    --edebug)
      bin='_typhoon'
      shift
      ;;
    --local)
      mode='local'
      shift
      ;;
    --icc)
      mode='ics'
      shift
      ;;
    --fics)
      mode='fics'
      ics='freechess.org'
      shift
      ;;
    *)
      usage $0
      ;;
  esac
done

case ${mode} in
  local)
    play_locally
    ;;
  ics)
    attach_to_ics
    ;;
  fics)
    attach_to_ics
    ;;
esac
exit 0
