## Minimal DynDNS updater

mini-ddns is a minimalistic and configurable ddns (Dynamic DNS) client, designed to be compatible with the most stripped down Linux routers.

It should work with most Linux devices, provided the kernel isn't too old (this was tested on an ADB modem/router, running Linux 3.4.11)

components:

- `ifwatch`: it listens for changes to network interfaces (by using `netlink`/`rtnetlink`) and publishes events in the format `[type]:[op]:[info]`.
It currently publishes the following event types:
  - `ip:add`
  - `ip:del`
  - `link:up`
  - `link:down`
  - `route:add`
  - `route:del`
 
- `monitor`: it implements a state machine which runs `ifwatch` and parses its output to determine when an IP/Route change has occurred, indicating that a new public IP address has been obtained.
It then runs a pre-defined command (`newip.sh`) with the new IP address as `argv[1]`

- `newip.sh`: the script implements the logic of updating the DynDNS information (by using curl, wget, or whatever is available on the system) with the new IP address, passed as the first argument `$1` by `monitor`
