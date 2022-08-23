# Lifeguard-bot
Lifeguard-bot is a security discord bot, scanning links and files for potential viruses.

## Sockets organisation

central.c
- receives links on port X
    - sends these links to scrapping-units or virus-checkers on port Y
- receives db requests on port Z
    - sends the end of the database on port Z
- receives "ready" on port Z

lifeguard-bot.py
- sends links to central on port X
- receives security audits on port A

scrapping-unit.py
- receives links from sites on port Y
- returns links found on port X
- sends "ready" to port Z

virus-checker.c
- receives virus links on port Y
    - sends security audits back to bot on port A


## Data structures
links on port X
- |priority (1 byte)|message id (8 bytes)|url (<512 bytes)|