eeg-toolkit
=================

## Get running in 5 minutes

First, check out eeg-toolkit and launch it locally:

```bash
git clone git@github.com:joshblum/eeg-toolkit.git
cd eeg-toolkit
```

You can either setup the repository to contribute to development or as a
standalone node to use the toolkit.

## Production setup

We recommend using the docker images to run a production node. Once the
repository is cloned you can run the following:

```bash
make docker-install
# log out and then log back in again for docker group permission changes to take affect
make docker-build
make docker-run
```

TODO(joshblum): add docker images to dockerhub
TODO(joshblum): add instructions for importing data

## Development setup

We have some installation commands that utilize `brew` or `apt-get`
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
make install
```
