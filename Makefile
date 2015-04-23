.PHONY: clean run installdeps lint pylint jslint prod-run install deploy

OS := $(shell uname)

ifeq ($(RPM),1)
	PKG_INSTALLER = yum
else
	PKG_INSTALLER = apt-get
endif

ifeq ('$(OS)', 'Darwin')
	OSX = true
	PKG_INSTALLER = brew
endif

# use `libs` directive for building shared library
-include compute/Makefile

clean:
	find . -type f -name '*.py[cod]' -delete
	find . -type f -name '*.*~' -delete

run: clean
	python server.py

pylint:
	-flake8 .

jslint:
	-jshint -c .jshintrc --exclude-path .jshintignore .

lint: clean pylint jslint

installdeps: clean
ifeq ('$(OSX)', 'true')
	# Run MacOS commands
	brew update
	cat packages-osx.txt | xargs brew install
	export PKG_CONFIG_PATH=/usr/local/Cellar/libffi/3.0.13/lib/pkgconfig/
else
	# Run Linux commands
	# setup gcc 4.7
	# TODO(joshblum): setup for yum as well
ifeq ('$(PKG_INSTALLER)', 'apt-get')
	sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
	sudo apt-get update
	sudo apt-get install -y gcc-4.9
	sudo apt-get install -y g++-4.9
	sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 20
	sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.9 20
	sudo update-alternatives --config gcc
	sudo update-alternatives --config g++
else
	sudo $(PKG_INSTALLER) update
endif
	cat packages.txt | xargs sudo $(PKG_INSTALLER) -y install
endif
	pip install -r requirements.txt

install: installdeps libs

deploy:
	fab prod deploy

prod-run: libs
	supervisorctl reread
	supervisorctl update
	supervisorctl restart eeg:eeg-8000
