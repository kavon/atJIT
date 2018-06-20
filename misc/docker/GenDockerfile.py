import yaml
import sys

Head = "# Dockerfile derived from easy::jit's .travis.yml"
From = "ubuntu:latest"
Manteiner = "Juan Manuel Martinez Caamaño jmartinezcaamao@gmail.com"
base_packages = ['build-essential', 'python', 'python-pip', 'git', 'wget', 'unzip', 'cmake']

travis = yaml.load(open(sys.argv[1]))
travis_sources = travis['addons']['apt']['sources']
travis_packages = travis['addons']['apt']['packages']
before_install = travis['before_install']
script = travis['script']

# I could not get a better way to do this
AddSourceCmd = {
  "llvm-toolchain-trusty-6.0" : "deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-6.0 main | tee -a /etc/apt/sources.list > /dev/null",
  "ubuntu-toolchain-r-test" : "apt-add-repository -y \"ppa:ubuntu-toolchain-r/test\""
}

Sources = ["RUN {cmd} \n".format(cmd=AddSourceCmd[source]) for source in travis_sources]

Apt = """# add sources
RUN apt-get update
RUN apt-get install -y software-properties-common
{AddSources}
# install apt packages, base first, then travis
RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y {base_packages} && \\
    apt-get install -y {travis_packages}
""".format(AddSources = "".join(Sources), base_packages = " ".join(base_packages), travis_packages=" ".join(travis_packages))

Checkout = "RUN git clone --depth=50 --branch=${branch} https://github.com/jmmartinez/easy-just-in-time.git easy-just-in-time && cd easy-just-in-time\n"
BeforeInstall = "".join(["RUN cd /easy-just-in-time && {0} \n".format(cmd) for cmd in before_install])
Run = "RUN cd easy-just-in-time && \\\n" + "".join(["  {cmd} && \\ \n".format(cmd=cmd) for cmd in script]) + "  echo ok!"

Template = """{Head}

FROM {From}

LABEL manteiner {Manteiner}

ARG branch=master

{Apt}
# checkout
{Checkout}
# install other deps
{BeforeInstall}
# compile and test!
{Run}"""

print(Template.format(Head=Head, From=From, Manteiner=Manteiner, Apt=Apt, BeforeInstall=BeforeInstall, Checkout=Checkout, Run=Run))
