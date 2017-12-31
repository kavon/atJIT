import generate
import re 


def on_include(args):
    args = args.split()
    filename = args[0]; 
    label = args[1]; 

    all_code = open(filename.strip()).read()

    inline = re.compile(".*// INLINE FROM HERE #"+ label +"#(?P<code>.*)// TO HERE #" + label + "#.*", flags=re.DOTALL)
    code = inline.match(all_code)
    code = code.group(1).rstrip().lstrip()

    print(code, end='')
    return

generate.match_and_expand("include", on_include)
