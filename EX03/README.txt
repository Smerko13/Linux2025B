This is the submission of assignment #3 for the Linux course taken in MTA 2025B by the following students:
Daniel Smerkovich
Netanel Mirel
Yael Moshkovich

In order to run and test the project, we will ask you to run the next command in your terminal:
sudo ./launcher.sh [num_of_decrypters]
The script will take care of the rest and what you will have eventually is one container for your encrypter and [num_of_decrypter] containers for your encrypters.

Please note:
*You will have to run the script with sudo privileges because as part of the project, we are asked to create directories in the root folder
*Each encrypter and decrypter will have log files both in the /var/log/ folder in the container and on the host machine as well!
*Password length is defined by a file located in /tmp/mtacrypt/ named mtacrypt.conf. The said file is created in the launcher script. If you wish to test the project with a different password length just change it in the script.
*Please wait when launching the script. Containers take time to load and we removed unnecessary console printing but the containers will run eventually.

Links to docker hub repositories:
https://hub.docker.com/repository/docker/smerko13/mta_crypt_encrypter/general
https://hub.docker.com/repository/docker/smerko13/mta_crypt_decrypter/general

We hope you have a fun time exploring this project.
Thank you for the semester!