eeg-toolkit
=================

## Get running in 5 minutes

First, check out eeg-toolkit and launch it locally:

```bash
git clone https://github.com/joshblum/eeg-toolkit.git
cd eeg-toolkit
```

We have some installation commands that utilize `brew` and `apt-get`
based on your system.

All Python packages are install using `pip` from the `requirements.txt` file.
If you wish to install via another method i.e. `conda`, all the packages and
versions can be found in `requirements.txt`.

The commands below install common packages for OSX or Linux and run the server.

Note: You need to use `sudo` if you are not working in a
[virtualenv](http://docs.python-guide.org/en/latest/dev/virtualenvs/) when
installing the dependencies.

```bash
# you should first lauch into your virtual env
make installdeps
make run
```
