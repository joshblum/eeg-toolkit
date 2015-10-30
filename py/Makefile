.PHONY: clean install

install:
ifeq ('$(shell uname)', 'Darwin')
	# Run MacOS commands
	brew update
	cat packages-osx.txt | xargs brew install
else
	# Run Linux commands
	# setup gcc 4.9
	sudo apt-get update
	cat packages.txt | xargs sudo apt-get -y install
endif
	pip install -r requirements.txt

clean:
	find . -type f -name '*.py[cod]' -delete
