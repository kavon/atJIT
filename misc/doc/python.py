import generate 

def on_python(python_code):
    exec (python_code) in {'__builtins__':{}}, {}
    return

generate.match_and_expand("python", on_python)
