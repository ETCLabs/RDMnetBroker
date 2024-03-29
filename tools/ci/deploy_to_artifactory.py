"""Deploys binaries to Artifactory with the specified version."""
import argparse
import os
import sys
import time
import requests

THIS_FILE_DIRECTORY = os.path.dirname(os.path.realpath(__file__))
ARTIFACTORY_API_KEY = os.getenv('RDMNETBRKR_ARTIFACTORY_API_KEY')
ARTIFACTORY_PROJECT_NAME = os.getenv('RDMNETBRKR_ARTIFACTORY_PROJECT_NAME')
ARTIFACTORY_URL = os.getenv('RDMNETBRKR_ARTIFACTORY_URL')

def upload_file(local_name: str, version: str):
    """Uploads the specified staged binary to Artifactory."""
    version_split = version.split('.')
    num_digits = len(version_split)
    if(num_digits < 4):
        print(f"The provided version {version} is invalid because it only has {num_digits} digits.")
        sys.exit(1)
    
    major_minor_patch = f'{version_split[0]}.{version_split[1]}.{version_split[2]}'

    local_name_split = local_name.split('.')
    remote_name = f'{local_name_split[0]}_v{version}.{local_name_split[1]}'

    headers = {
        'X-JFrog-Art-Api': f"{ARTIFACTORY_API_KEY}",
    }

    with open(local_name, 'rb') as f:
        data = f.read()

    response = requests.put(f'{ARTIFACTORY_URL}/{major_minor_patch}/{remote_name};project={ARTIFACTORY_PROJECT_NAME};version={version}', headers=headers, data=data)
    if (response.status_code == 201):
        print(f"Successfully uploaded {local_name}.")
    else:
        print(f"Failed to upload {local_name}, received status code {response.status_code}")
        sys.exit(1)


def deploy_binaries(version: str):
    """Deploys all staged binaries to Artifactory."""
    upload_file("RDMnetBroker_x86.msi", version)
    upload_file("RDMnetBroker_x86.msm", version)
    upload_file("RDMnetBroker_x64.msi", version)
    upload_file("RDMnetBroker_x64.msm", version)
    upload_file("RDMnetBroker.pkg", version)


def main():
    parser = argparse.ArgumentParser(
        description="Deploy RDMnet Broker artifacts to Artifactory"
    )
    parser.add_argument("version", help="Artifact version being deployed")
    args = parser.parse_args()

    # Make sure our cwd is the root of the repository
    os.chdir(os.path.abspath(os.path.join(THIS_FILE_DIRECTORY, "..", "..")))

    deploy_binaries(args.version)


if __name__ == "__main__":
    main()
