# BananaOS Direct Link

On the handheld the app is **Direct Link**. It puts two BananaOS handhelds on their own Wi-Fi so a game can use local multiplayer without a router. BananaDS LAN, RetroArch and PPSSPP can use it. The host is `10.42.0.1`. The other handheld takes the first free address from `10.42.0.2` to `10.42.0.9`.

The link stays up after you leave the app and while you change games. Disconnect brings the home Wi-Fi back. There is no internet while the link is up.

The cell is open. BananaOS's wpa_supplicant is built without encrypted ad-hoc (IBSS RSN), so anyone in range can join. This is a local link, not a private network.

This tree has not been tried on a handheld yet.

## Install

The zip contains `direct-link/`, `Direct Link.sh` and `INSTALL.txt`. Copy the folder to `store/apps/` and the tile to `store/tiles/`. Do not replace `store`. Reboot once, then open Apps. The full steps are in `INSTALL.txt`.

Devices: Anbernic RG DS and RG DS Plus, BananaOS 0.5.0.1.

## Uninstall

In the app, Y, then A. That disconnects and deletes `store/apps/direct-link` and `store/tiles/Direct Link.sh`.

## Settings

`/data/banana/direct-link.conf` is optional. It is read, never executed. One `KEY=value` per line: `CHANNEL` (1–11), `FREQ` (MHz, wins over `CHANNEL`), `SSID`, `BSSID`, `MAX` (seconds; `0` means until Disconnect).

## Build

`./build/package.sh` compiles the on-screen app and writes `dist/direct-link-<version>.zip`. Set `CC` to an aarch64 gcc whose sysroot has libdrm, or have the `bananads-build:1` image available. `./build/selftest.sh` checks the scripts without a handheld.

## License

MIT. `app/wifi-link` is adapted from BananaLinux. See `LICENSE` and `NOTICE`.

---

# BananaOS Direct Link

En la consola la app se llama **Direct Link**. Pone dos consolas BananaOS en su propio Wi-Fi para que un juego use el multijugador local sin router. Sirve para BananaDS LAN, RetroArch y PPSSPP. El host es `10.42.0.1`. La otra consola toma la primera dirección libre entre `10.42.0.2` y `10.42.0.9`.

El enlace sigue activo al salir de la app y al cambiar de juego. Disconnect devuelve el Wi-Fi de casa. Mientras el enlace está activo no hay internet.

La red es abierta. El wpa_supplicant de BananaOS no trae ad hoc cifrado (IBSS RSN), así que cualquiera al alcance puede unirse. Es un enlace local, no una red privada.

Este árbol todavía no se probó en una consola.

## Instalación

El zip trae `direct-link/`, `Direct Link.sh` e `INSTALL.txt`. Copia la carpeta a `store/apps/` y el tile a `store/tiles/`. No reemplaces `store`. Reinicia una vez y abre Apps. Los pasos completos están en `INSTALL.txt`.

Equipos: Anbernic RG DS y RG DS Plus, BananaOS 0.5.0.1.

## Desinstalar

En la app, Y y luego A. Eso desconecta y borra `store/apps/direct-link` y `store/tiles/Direct Link.sh`.
