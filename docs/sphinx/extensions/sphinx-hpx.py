# Copyright (c) 2018 Mikael Simberg
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

from docutils import nodes

def setup(app):
    app.add_role('hpx-issue', autolink('https://github.com/STEllAR-GROUP/hpx/issues/%s', "Issue #"))
    app.add_role('hpx-header', autolink_hpx_file('http://github.com/STEllAR-GROUP/hpx/blob/%s/%s/%s'))
    app.add_role('hpx-pr', autolink('https://github.com/STEllAR-GROUP/hpx/pull/%s', "PR #"))
    app.add_role('cppreference-header', autolink('http://en.cppreference.com/w/cpp/header/%s'))
    app.add_role('cppreference-algorithm', autolink('http://en.cppreference.com/w/cpp/algorithm/%s'))
    app.add_role('cppreference-memory', autolink('http://en.cppreference.com/w/cpp/memory/%s'))
    app.add_role('cppreference-container', autolink('http://en.cppreference.com/w/cpp/container/%s'))
    app.add_role('cppreference', autolink_generic('http://en.cppreference.com/w/cpp/%s/%s'))

def autolink(pattern, prefix=''):
    def role(name, rawtext, text, lineno, inliner, options={}, content=[]):
        url = pattern % (text,)
        node = nodes.reference(rawtext, prefix + text, refuri=url, **options)
        return [node], []
    return role

# The text in the rst file should be:
# :hpx-header:`base_path,file_name`
def autolink_hpx_file(pattern):
    def role(name, rawtext, text, lineno, inliner, options={}, content=[]):
        text_parts = [p.strip() for p in text.split(',')]
        commit = inliner.document.settings.env.app.config.html_context['fullcommit']
        url = pattern % (commit, text_parts[0], text_parts[1])
        node = nodes.reference(rawtext, text_parts[1], refuri=url, **options)
        return [node], []
    return role

# The text in the rst file should be:
# :cppreference:`base_path,typename`, for instance `thread,barrier`
def autolink_generic(pattern):
    def role(name, rawtext, text, lineno, inliner, options={}, content=[]):
        text_parts = [p.strip() for p in text.split(',')]
        url = pattern % (text_parts[0], text_parts[1])
        node = nodes.reference(rawtext, "std::" + text_parts[1], refuri=url, **options)
        return [node], []
    return role
