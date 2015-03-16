from fabric.api import env, cd, run


def prod():
  env.user = 'joshblum'
  env.gateway = 'tre.ef.technion.ac.il'
  env.forward_agent = True
  env.hosts = ['gpu-os02.ef.technion.ac.il']
  env.key_filename = '~/.ssh/id_rsa.pub'
  env.server_path = '~/eeg-spectrogram'
  env.python_path = '~/.virtualenvs/eeg-spectrogram'
  return


def deploy():
  with cd(env.server_path):
    run('git pull --rebase origin master')
    run('workon eeg-spectrogram')
    run('make install')
    run('make prod-run')
