# Lifeguard-bot

Lifeguard-bot is a security discord bot, scanning links and files for potential viruses.

```diff
- Do not use this bot now, it's not finished, it returns A LOT of false positives.
```

## TODO list

- ~~investigate the bug that makes lifeguard-bot.py stop working after a while, while still being online.~~ DONE
- ~~add an exception for images analysis~~ DONE
- ~~change the database system by a most flexible one~~ DONE
- ~~add scrapping_unit.c program~~ DONE
- ~~modify the returned embed to quote the filename~~ DONE
- ~~change queue.c to handle priority numbers~~ DONE
- ~~redirect to the fastest server each time~~ DONE
- ~~investigate the bug why virus_checker receive 0 bytes when we send many files in discord~~ DONE
- ~~improve the gestion of http redirections~~ DONE
- ~~improve the way we differenciate files and websites (content-type)~~ DONE
- add a virustotal verification to reduce false positive
    - if virustotal returns "virus"
        - probability = 100%
    - if virustotal returns "known distributor"
        - probability = 0%
    - if virustotal returns "0 detection"
        - probability = lambda * p,  with 0 < lambda < 1

## Sockets organisation

central.c
- receives "ready" on port INFOS_PORT and memorizes ip and port
- receives links on port UNKNOW_LINKS_PORT
    - sends these links to scrapping-units or virus-checkers on their respective ports

lifeguard-bot.py
- sends links to central on port UNKNOW_LINKS_PORT
- receives security audits on port AUDIT_PORT

scrapping-unit.c
- binds on port 0 and sends "ready" with port binded to port INFOS_PORT
- receives links from sites on port binded
- returns links found on port UNKNOW_LINKS_PORT


virus-checker.c
- binds on port 0 and sends "ready" with port binded to port INFOS_PORT
- receives virus links on port binded
    - sends security audits back to bot on port AUDIT_PORT
