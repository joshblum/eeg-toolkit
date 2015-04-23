from fabric.api import env, cd, run


def prod():
  env.user = 'ubuntu'
  env.hosts = ['128.52.175.133', '128.52.178.57']
  env.key_filename = '~/.ssh/id_rsa.pub'
  env.server_path = '~/eeg-spectrogram'
  env.python_path = '~/.virtualenvs/eeg-spectrogram'
  return


def deploy():
  with cd(env.server_path):
    run('git pull --rebase origin master')
    run('workon eeg-spectrogram; make install')
    run('workon eeg-spectrogram; make prod-run')
