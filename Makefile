.PHONY: clean run installdeps lint pylint jslint

OS := $(shell uname)

ifeq ($(RPM),1)
	PKG_INSTALLER = yum
else
	PKG_INSTALLER = apt-get
endif

run: clean
	python server.py

clean:
	find . -type f -name '*.py[cod]' -delete
	find . -type f -name '*.*~' -delete

pylint:
	-flake8 .

jslint:
	-jshint -c .jshintrc --exclude-path .jshintignore .

lint: clean pylint jslint

installdeps:
ifeq ('$(OS)','Darwin')
	# Run MacOS commands
	cat packages-osx.txt | xargs brew install
	export PKG_CONFIG_PATH=/usr/local/Cellar/libffi/3.0.13/lib/pkgconfig/
else
	# Run Linux commands
	cat packages.txt | xargs sudo $(PKG_INSTALLER) -y install
endif
	pip install -r requirements.txt
