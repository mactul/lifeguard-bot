killall central
killall virus_checker
pkill -f lifeguard-bot.py

./central >/dev/null &
sleep 2
./virus_checker >/dev/null &
python3 lifeguard-bot.py >/dev/null &

disown
