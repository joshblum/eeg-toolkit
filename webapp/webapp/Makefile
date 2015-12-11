.PHONY: installdeps\
	clean

installdeps:
	sudo apt-get update
	cat packages.txt | xargs sudo apt-get -y install
	pip install -r requirements.txt

clean:
	find . -type f -name '*.py[cod]' -delete
