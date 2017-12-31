import re
import sys

def get_split(tag):
    start = re.compile("</"+tag)
    end = re.compile("/>")
    def split(contents):
        start_match = start.search(contents, pos=0)
        if not start_match:
            return contents, "", "", False
        end_match = end.search(contents, pos=start_match.end())
        if not end_match:
            return contents, "", "", False
        pre = contents[0 : start_match.start()]
        args = contents[start_match.end() : end_match.start()]
        post = contents[end_match.end() : ] 
        return pre, args, post, True

    return split

def match_and_expand(tag, expand_with):
    do_match_and_expand(sys.stdin.read(), get_split(tag), expand_with)
    return

def do_match_and_expand(contents, split, expand_with):
    pre, code, post, match = split(contents)
    if match :
        do_match_and_expand(pre, split, expand_with)
        expand_with(code)
        do_match_and_expand(post, split, expand_with)
    else:
        print(contents, end='')
