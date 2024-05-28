# <p align="center">ft_otp</p>
> *Projet d'introduction à la notion de mot de passe à usage unique (OTP) utilisant le RFC HOTP.*
>
> *Les mots de passe sont l'un des plus gros casse-tête de la sécurité informatique. Les utilisateurs les oublient, les partagent, les réutilisent et en choisissent de très mauvais. De plus, les mots de passe finissent tôt ou tard par être divulgués lors de failles de sécurité.
>
> Une façon d'éviter cela est d'utiliser des mots de passe à usage unique, basés sur des horodatages, qui expirent après quelques minutes et deviennent alors invalides. Que vous utilisiez déjà ce système ou que vous n'en ayez jamais entendu parler, il est fort probable qu'un de vos mots de passe ait déjà été compromis à un moment de votre vie.*
>
> *Ce projet vise à implémenter un système de mot de passe à usage unique basé sur le temps (TOTP). Ce système sera capable de générer des mots de passe temporaires et uniques à partir d'une clé principale. Il sera basé sur la [RFC-6238](https://datatracker.ietf.org/doc/html/rfc6238), ce qui vous permettra de l'utiliser au quotidien.*

## Install
```bash
pip install -r requirements.txt
```

## Usage
```bash
./main.py -g <64 hex bytes>
# exemple: ./main.py -g $(for _ in {1..64}; do echo -n f; done)
./main.py -ik
# or ./main.py -qik
```

