DMX zu KNX Gateway
==================

Anleitung zur Installation des DMX-KNX-Gateways zur Ansteuerung des Saalichts (KNX = EIB - Bus) über den DMX-Bus.

Hardware & OS
-------------
Basis des Gateways ist ein Raspberry Pi - Computer mit ArchlinuxARM auf der SD-Karte.
Raspberry Pi Hardware: Revision 2 Model B (512MB RAM)

Zuerst muss über ein USB auf 3.3V UART-Kabel eine Verbindung zum Raspberry Pi aufgenommen werden. Der Raspberry Pi sollte per Netzwerkkabel ans Internet und an einen DHCP-Server (Router) angebunden sein.
Login auf dem Pi mit Benutzername `root` und Passwort `root` im serielle-Schnittstelle-Terminal. Nun kann OpenSSH per `pacman -S openssh` installiert werden. SSH wird durch den Befehl
`systemctl enable sshd` aktiviert. Nach einem Neustart per `reboot` wird zum Raspberry Pi wird via SSH eine Verbindung aufgenommen. Alle folgenden Befehle werden auf dem Raspberry Pi ausgeführt.
(Tipp: Um die IP - Adresse des Raspberry Pis herauszufinden, noch im seriellen Terminal `ip addr` ausführen).

Software installieren
---------------------
Die Installation der Software übernimmt das script `install.sh`. Dieser Ordner muss dazu z.B. per scp oder per git clone auf den Raspberry Pi gebracht werden. Das Script installiert alle für den Betrieb notwendigen Komponenten und macht die SD-Karte nur lesbar.

EIBd kann testweise gestartet werden durch:

    eibd tpuarts:/dev/ttyACM0 -i

Außerdem können Werte auf Adressen geschrieben werden mithilfe von:

    groupswrite ip:127.0.0.1 1/2/3 1

1/2/3 steht hier für die KNX-Adresse, 1 steht für den zu schreibenden Wert.
