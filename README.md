# Lifeguard-bot

Lifeguard-bot is a security discord bot, scanning links and files for potential viruses.

Do not use this bot now, it's not finished, it returns A LOT of false positives.

## TODO list

- investigate the bug that makes lifeguard-bot.py stop working after a while, while still being online.
- change db2.bin to a sorted one and use dichotomial search by size to found the best malware correspondance.
- add scrapping_unit.py program
- change queue.c to handle priority numbers
- add a virustotal verification to reduce false positive
    - if virustotal returns "virus"
        - probability = 100%
    - if virustotal returns "known distributor"
        - probability = 0%
    - if virustotal returns "0 detection"
        - probability = lambda * p,  with 0 < lambda < 1

## Sockets organisation

central.c
- receives db requests on port INFOS_PORT
    - sends the end of the database on port INFOS_PORT
- receives "ready" on port INFOS_PORT and memorizes ip and port
- receives links on port UNKNOW_LINKS_PORT
    - sends these links to scrapping-units or virus-checkers on their respective ports

lifeguard-bot.py
- sends links to central on port UNKNOW_LINKS_PORT
- receives security audits on port AUDIT_PORT

scrapping-unit.py
- binds on port 0 and sends "ready" with port binded to port INFOS_PORT
- receives links from sites on port binded
- returns links found on port UNKNOW_LINKS_PORT


virus-checker.c
- requests the end of the database on port INFOS_PORT
- binds on port 0 and sends "ready" with port binded to port INFOS_PORT
- receives virus links on port binded
    - sends security audits back to bot on port AUDIT_PORT
