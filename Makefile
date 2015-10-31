.PHONY: clean ws_server run installdeps lint pylint jslint libs prod-run install deploy submodules submodule-update dev-packages docker-install docker-run docker-stop docker-push

DOCKER_WEBAPP_NAME := "joshblum/eeg-toolkit-webapp"
DOCKER_TOOLKIT_NAME := "joshblum/eeg-toolkit-toolkit"

default: ws_server

libs:
	make -C toolkit/toolkit/compute/ libs

ws_server:
	make -C toolkit/toolkit/ ws_server

submodule-update:
	git submodule foreach git checkout master; git pull

submodules:
	git submodule update --init --recursive

installdeps: clean submodules dev-packages
	make -C toolkit/toolkit installdeps
	make -C webapp/webapp installdeps

dev-packages:
ifeq ('$(OSX)', 'true')
	cat packages-dev-osx.txt | xargs brew install
else
	# Run Linux commands
	sudo apt-get update
	cat packages-dev.txt | xargs sudo apt-get -y install
endif
	pip install -r requirements.txt

install: installdeps ws_server

docker-install:
	curl -sSL https://get.docker.com/ | sh
	sudo usermod -aG docker ubuntu


docker-build: clean submodules
	-cd webapp && docker build -t $(DOCKER_WEBAPP_NAME):latest .
	-cd toolkit && docker build -t $(DOCKER_TOOLKIT_NAME):latest .

docker-run:
	-docker run -d -p 5000:5000 --name=$(DOCKER_WEBAPP_NAME) $(DOCKER_WEBAPP_NAME)
	-docker run -d -p 8080:8080 --name=$(DOCKER_TOOLKIT_NAME) -v /home/ubuntu/MIT-EDFs:/home/ubuntu/MIT-EDFs $(DOCKER_TOOLKIT_NAME)

docker-stop:
	-docker stop $(DOCKER_WEBAPP_NAME)
	-docker stop $(DOCKER_TOOLKIT_NAME)

docker-rm:
	-docker rm $(DOCKER_WEBAPP_NAME)
	-docker rm  $(DOCKER_TOOLKIT_NAME)

docker-push: docker-build
	-docker push $(DOCKER_WEBAPP_NAME):latest
	-docker push $(DOCKER_TOOLKIT_NAME):latest

deploy:
	fab prod deploy

run: ws_server
	./toolkit/toolkit/ws_server 8080 & \
		python webapp/webapp/server.py

prod-run: clean ws_server
	supervisorctl reread
	supervisorctl update
	supervisorctl restart eeg:eeg
	supervisorctl restart ws:ws

pylint:
	-flake8 .

jslint:
	-jshint -c .jshintrc --exclude-path .jshintignore .

lint: clean pylint jslint

clean:
	find . -type f -name '*.py[cod]' -delete
	find . -type f -name '*.*~' -delete
	find . -type f -name 'main' -delete
	find . -type f -name 'lib_eeg_spectrogram.so' -delete
	find . -type f -name 'ws_server' -delete
	find . -type f -name '*.[dSYM|o|d]' -delete
