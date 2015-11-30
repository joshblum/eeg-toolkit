.PHONY: clean\
	ws_server\
	submodule-update\
	submodules\
	installdeps\
	dev-packages\
	mount-volume\
	install\
	docker-install\
	docker-run\
	docker-stop\
	docker-push\
	deploy\
	pylint\
	jslint\
	lint

REPO := "joshblum"
DOCKER_WEBAPP_NAME := "eeg-toolkit-webapp"
DOCKER_TOOLKIT_NAME := "eeg-toolkit-toolkit"

MOUNT_POINT := "home/ubuntu/eeg-data/eeg-data"
DEVICE := "/dev/vdb"

default: ws_server

ws_server:
	make -C toolkit/toolkit/ ws_server

submodules:
	git submodule update --init --recursive

submodule-update:
	git submodule foreach git checkout master
	git submodule foreach git pull --rebase origin master

installdeps: clean submodules dev-packages
	make -C toolkit/toolkit installdeps
	make -C toolkit/toolkit/storage/TileDB
	make -C webapp/webapp installdeps

dev-packages:
	sudo apt-get update
	cat dev-packages.txt | xargs sudo apt-get -y install
	pip install -r requirements.txt

mount-volume:
	mkfs.ext4 $(DEVICE)
	mkdir -p $(MOUNT_POINT)
	mount $(DEVICE) $(MOUNT_POINT)
	sudo chown -R ubuntu:ubuntu $(MOUNT_POINT)

install: installdeps ws_server

docker-install:
	curl -sSL https://get.docker.com/ | sh
	sudo usermod -aG docker ubuntu

docker-build: clean submodules
	-cd webapp && docker build -t $(REPO)/$(DOCKER_WEBAPP_NAME):latest .
	-cd toolkit && docker build -t $(REPO)/$(DOCKER_TOOLKIT_NAME):latest .

docker-run:
	-docker run -d -p 5000:5000 --name=$(DOCKER_WEBAPP_NAME) $(REPO)/$(DOCKER_WEBAPP_NAME)
	-docker run -d -p 8080:8080 --name=$(DOCKER_TOOLKIT_NAME) -v $(MOUNT_POINT):$(MOUNT_POINT) $(REPO)/$(DOCKER_TOOLKIT_NAME)

docker-stop:
	-docker stop $(DOCKER_WEBAPP_NAME)
	-docker stop $(DOCKER_TOOLKIT_NAME)

docker-rm:
	-docker rm $(DOCKER_WEBAPP_NAME)
	-docker rm  $(DOCKER_TOOLKIT_NAME)

docker-push: docker-build
	-docker push $(REPO)/$(DOCKER_WEBAPP_NAME):latest
	-docker push $(REPO)/$(DOCKER_TOOLKIT_NAME):latest

deploy:
	fab env prod_deploy

pylint:
	-flake8 .

jslint:
	-jshint -c .jshintrc --exclude-path .jshintignore .

lint: clean pylint jslint

clean:
	make -C toolkit/toolkit/ clean
	make -C webapp/webapp/ clean
