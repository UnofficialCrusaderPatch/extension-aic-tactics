"""Private reference-image identities, never imported by the module runtime."""
import hashlib

HASHES={
    'SHC':{
        '3bb0a8c1e72331b3a30a5aa93ed94beca0081b476b04c1960e26d5b45387ac5a':'local',
        '0d3d0d0be90a41d0c07d02cb41e6edc3e399288d16039db5b666392660fbda34':'EFIGS',
        '2aab6b3da99148b0796bd00a92b4b19db7548d1e2c50fa4372035f716fd33cab':'PL',
    },
    'SHCE':{
        '55648e6b05d67d37a5773fe699bbb17a2d6ad4de1bb9dbded9a21caef82bd7fb':'local',
        '70f083211e4260d877979e29bf5f5420aaa1c69fc5ee47057be458a84b5e6d0d':'EFIGS',
        'e7e82625a39d3840bf44a84456967eeecafe7ec9d716afa67f1856ad59a9d460':'PL',
    },
}


def digest(raw,variant):
    value=hashlib.sha256(raw).hexdigest()
    assert value in HASHES[variant], 'Unknown research executable: '+value
    return value
