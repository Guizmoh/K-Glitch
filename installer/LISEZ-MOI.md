# Installer K-GLITCH 0.10.0 sur le M-VAVE FM-1

Tout est dans ce dossier : l'installateur contient le firmware, il marche **sans internet**.

## Ce qu'il faut

- Le FM-1, un **câble USB qui transporte les données** (pas un câble de charge seule), branché **directement** sur l'ordinateur.
- **Google Chrome** ou **Microsoft Edge** sur ordinateur (Windows, Mac ou Linux). Safari et Firefox ne savent pas parler MIDI à une page web.
- Fermez les applications qui utilisent le MIDI (DAW, éditeur SLOOP, autre onglet d'installateur…).

## Installer

1. Branchez le FM-1 en USB et allumez-le (avec le firmware d'origine, SLOOP ou une ancienne version de K-GLITCH).
2. Ouvrez l'installateur :
   - **Windows** : double-cliquez sur `INSTALLER-WINDOWS.bat` (il ouvre la page dans Chrome ou Edge) ;
   - **Mac** : double-cliquez sur `INSTALLER-MAC.command` (la première fois : clic droit > Ouvrir) ;
   - ou ouvrez directement `K-GLITCH-0.10.0-installer.html` avec Chrome ou Edge.
3. La page affiche « K-GLITCH 0.10.0 (SLOOP 2.8.4) » : le firmware est chargé et vérifié.
4. Cliquez sur **Installer**, puis acceptez l'accès MIDI (« contrôle total des appareils MIDI ») que demande le navigateur.
5. Attendez « Terminé » (environ une minute) sans débrancher le câble. Le FM-1 redémarre sur K-GLITCH.

Si l'écriture est coupée (câble débranché, page fermée), le FM-1 reste en mode mise à jour : rebranchez-le et cliquez à nouveau sur Installer, l'installation reprend.

## En ligne de commande (autre méthode)

Avec Python 3 et `pip install mido python-rtmidi` :

```
python3 fm1_install.py k-glitch-0.10.0.fwsc
```

## Revenir en arrière

- **Vers SLOOP** : installez le paquet SLOOP de votre choix avec son propre installateur.
- **Vers le firmware officiel** : l'outil M-UPGRADE de M-VAVE (www.m-vave.com/download : PC Software > M-UPGRADE, et PC Firmware > FM-1).
- Le firmware officiel ne peut pas ouvrir les morceaux, projets et samples de SLOOP / K-GLITCH.

## Bon à savoir

- Version de développement : construite et vérifiée par la chaîne de compilation officielle de JieLi (taille, mémoire, accès matériel), testée sur ordinateur et dans l'émulateur ; les versions 0.9 ont tourné sur un FM-1 réel, **la 0.10 pas encore**. À vos risques.
- Si le FM-1 ne démarre plus du tout après une installation ratée : maintenez **OCT−** en l'allumant (mode de secours USB), puis relancez l'installateur. En dernier recours, un Transporter (github.com/kurogedelic/FM-1-transporter) permet de le récupérer.
- Paquet : `k-glitch-0.10.0.fwsc`, identité FM-1_909, SHA-256 `2c4fb2a70fa12fa3bda705345b745be8c633697f0471fa6d30f1f28fa9ba94e7`.
