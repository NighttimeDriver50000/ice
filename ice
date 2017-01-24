#!/usr/bin/env python3

# Basic imports: system, OS, regular expressions
import sys, os, re
# Safe path joining
from os.path import join as pjoin
# Canonical paths
from os.path import realpath
# High-level file operations
import shutil
# Unix-like wildcards for paths
import glob
# Command-line option parsing
import argparse
# Useful iteration utilities
import itertools
# Iterable concatenation
chainfi = itertools.chain.from_iterable
# OS file stat interpretation
import stat
# Streams
import io
# Tar archive format
import tarfile
# Deep object copying
import copy
# Multithreading
import threading

# The directory that contains the ice executable
DIRNAME0 = os.path.dirname(realpath(__file__))
# The ice-files directory
ICE_FILES = pjoin(DIRNAME0, 'ice-files')
# Base makefile for the iRobot Create
IROBOT_MAKEFILE = pjoin(ICE_FILES, 'iRobot-Makefile')
# Base configuration fo YouCompleteMe Vim code completion
YCM_EXTRA_CONF = pjoin(ICE_FILES, 'ycm_extra_conf.py')
# Utils folder
UTILS_DIR = pjoin(ICE_FILES, 'utils')
# Default main.c
MAIN_C = pjoin(ICE_FILES, 'main.c')

class Context (object):
    '''Wrapper for the (slightly processed) program arguments.'''
    subcommand = project_paths = port_override = sources = includes = args = None


class IceError (Exception):
    def __str__(self):
        return 'ice: ERROR: {}'.format(Exception.__str__(self))

def warn(message):
    print('ice: WARNING: {}'.format(message))


def determine_avrdude_port(context):
    '''Determine the port for avrdude.

        If port_override is set and is a valid character special device file,
        or both port_override and force_port are set, then return port_override.
        Otherwise if port_override is set and the other conditions are not true,
        error out.

        If port_override is not set, glob /dev/ttyUSB*. If there is only one
        result, return it. Otherwise error out.
    '''
    port_override = context.port_override
    # Port Overriding
    if port_override is not None:
        # Determine if the given override is a character special device file
        if not (os.path.exists(port_override)
                and stat.S_ISCHR(os.stat(port_override).st_mode)):
            warn('"{}" is not a character special device file!'
                    .format(port_override))
        return port_override
    # Dynamic Port
    tty_usb_glob = glob.glob('/dev/ttyUSB*')
    # Success
    if len(tty_usb_glob) == 1:
        return tty_usb_glob[0]
    # Failures
    elif len(tty_usb_glob) == 0:
        raise IceError('No tty port found for avrdude! Input one with the --port option.')
    else:
        print('ice: ERROR: Multiple tty ports found for avrdude:', file=sys.stderr)
        for tty_usb in tty_usb_glob:
            print('{}{}'.format(' '*16, tty_usb), file=sys.stderr)
        raise IceError('  Select one with the --port option.')

# Matches the main function in C
main_re = re.compile(r'^\s*(int|void)\s*main\s*\(')

def has_main(filepath):
    with open(filepath) as f:
        for line in f:
            if main_re.match(line):
                return True
    return False

def get_target(context, project_path):
    if context.target is None:
        globbed_paths = glob_paths(context.sources, project_path)
        sources = [realpath(pjoin(project_path, src)) for src in globbed_paths]
        sources.sort()
        for source in sources:
            if has_main(source):
                return os.path.relpath(source, project_path)[:-2]
        raise IceError('No main function found!')
    else:
        target_file = pjoin(project_path, context.target + '.c')
        if not os.path.exists(target_file):
            warn('"{}" does not exist!'.format(target_file))
        elif not os.path.isfile(target_file):
            warn('"{}" is not a file!'.format(target_file))
        elif not has_main(target_file):
            warn('"{}" has no main function!'.format(target_file))
        return context.target

def glob_paths(paths, project_path):
    '''Glob paths within a project.
    
        E.g. glob_paths(['./*.c', './utils/*.c'], 'Project01')
                                --> ['main.c', 'other.c', 'utils/utils.c']
    '''
    globbed_paths = []
    # Make the project path absolute
    real_proj_path = realpath(project_path)
    for globable in paths:
        # Make the glob absolute
        real_globable = pjoin(glob.escape(real_proj_path), globable)
        # Glob it
        globs = glob.glob(real_globable)
        # Make the globbed paths relative
        globbed_paths.extend([os.path.relpath(realpath(g),
            real_proj_path) for g in globs])
    return globbed_paths


def check_frozen(project_path):
    frozen = os.path.exists(pjoin(project_path, '.FROZEN'))
    if frozen:
        raise IceError('"{}" is frozen.'.format(project_path))


# Dictionary of fields to be edited in the Makefile
# Format is FIELD: (isProjectSpecific, function)
edited_fields = {
        'DEBUG': (0, lambda context: 'stabs'),
        'AVRDUDE_PROGRAMMER': (0, lambda context: 'stk500v1'),
        'AVRDUDE_PORT': (0, determine_avrdude_port),
        'TARGET': (1, get_target),
        'SRC': (1, lambda context, project_path: ' '.join(glob_paths(context.sources, project_path))),
        'EXTRAINCDIRS': (1, lambda context, project_path: ' '.join(glob_paths(context.includes, project_path))) }

# Matches lines with one of the to-be-edited fields on them
makefile_line_re = re.compile(r'^(' + r'|'.join(edited_fields.keys()) + r') = (.*)$')

def filter_irobot_makefile_line(line, field_edits, do_print=False):
    # Attempt to match each line
    m = makefile_line_re.match(line)
    if m:
        # Make edits
        key = m.group(1)
        value = field_edits[key]
        new_line = '{} = {}\n'.format(key, value)
        if do_print:
            print('  {}'.format(new_line), end='')
        return new_line
    else:
        # Pass through unchanged
        return line

def refresh_makefile(context):
    '''Rebuild the makefile(s) for the project(s).'''
    # Generate the values for the non-project-specific edits
    _field_edits = {}
    for (key, func) in edited_fields.items():
        if not func[0]:
            _field_edits[key] = func[1](context)
    for project_path in context.project_paths:
        try:
            check_frozen(project_path)
            # Path for the project's Makefile
            makefile = realpath(pjoin(project_path, 'Makefile'))
            print(makefile)
            # Generate the values for the project-specific edits
            field_edits = dict(_field_edits)
            for (key, func) in edited_fields.items():
                if func[0]:
                    field_edits[key] = func[1](context, project_path)
            # Open source and destination makefiles 
            files_different = False
            with open(makefile, 'r') as f:
                for line in open(IROBOT_MAKEFILE, 'r'):
                    if f.readline() != filter_irobot_makefile_line(line, field_edits):
                        files_different = True
                        break
            # Open source and destination makefiles
            if files_different:
                with open(makefile, 'w') as f:
                    for line in open(IROBOT_MAKEFILE, 'r'):
                        f.writelines([filter_irobot_makefile_line(line, field_edits, True)])
            else:
                print('  No changes necessary.')
        except IceError as e:
            print(e, file=sys.stderr)

# Matches the insert point line in the YCM file.
ycm_line_re = re.compile(r'^\s*##+!!+ICE_INSERT_POINT')

def filter_ycm_line(line, new_lines, do_print=False):
    # Attempt to match each line
    m = ycm_line_re.match(line)
    if m:
        # Make insert
        if do_print:
            for new_line in new_lines:
                print('  {}'.format(new_line), end='')
        return new_lines
    else:
        return [line]

def refresh_ycm(context):
    '''Refresh the YCM config file(s) for the project(s).'''
    for project_path in context.project_paths:
        try:
            check_frozen(project_path)
            # Path for the project's ycm config
            ycm_file = realpath(pjoin(project_path, '.ycm_extra_conf.py'))
            print(ycm_file)
            # Generate the include cargs
            include_paths = glob_paths(context.includes, project_path)
            include_args = chainfi(map(lambda path: ('-I', path), include_paths))
            new_lines = list(map(lambda arg: '{!r},\n'.format(arg), include_args))
            # Open source and destination files
            files_different = False
            with open(ycm_file, 'r') as f:
                for line in open(YCM_EXTRA_CONF, 'r'):
                    for line2 in filter_ycm_line(line, new_lines):
                        if f.readline() != line2:
                            files_different = False
                            break
                    if files_different:
                        break
            # Open source and destination files
            if files_different:
                with open(ycm_file, 'w') as f:
                    for line in open(YCM_EXTRA_CONF, 'r'):
                        f.writelines(filter_ycm_line(line, new_lines, True))
            else:
                print('  No changes necessary.')
        except IceError as e:
            print(e, file=sys.stderr)


def copytree_force(src, dst):
    try:
        shutil.rmtree(dst)
    except FileNotFoundError:
        pass
    shutil.copytree(src, dst)


def download_utils(context):
    '''Refresh (download) the utils folder for the project(s).'''
    for project_path in context.project_paths:
        try:
            check_frozen(project_path)
            project_utils = pjoin(project_path, 'utils')
            print(project_utils)
            copytree_force(UTILS_DIR, project_utils)
            project_bk = pjoin(project_path, '.utils.bk')
            print(project_bk)
            copytree_force(project_utils, project_bk)
        except IceError as e:
            print(e, file=sys.stderr)


def upload_utils(context):
    '''Upload the utils folder for the project.
    
        Only valid for one project.
    '''
    if len(context.project_paths) != 1:
        raise IceError('--sync-utils is only valid for one project!')
    project_path = context.project_paths[0]
    try:
        check_frozen(project_path)
        project_utils = pjoin(project_path, 'utils')
        print(UTILS_DIR)
        copytree_force(project_utils, UTILS_DIR)
        project_bk = pjoin(project_path, '.utils.bk')
        print(project_bk)
        copytree_force(project_utils, project_bk)
    except IceError as e:
        print(e, file=sys.stderr)


def dirs_equal(left, right):
    import filecmp
    def _dc_equal(dc):
        if dc.left_only or dc.right_only or dc.diff_files or dc.funny_files:
            return False
        return all(map(_dc_equal, dc.subdirs.values())) if dc.subdirs else True
    return _dc_equal(filecmp.dircmp(left, right))


def sync_utils(context):
    '''Upload the project utilities folder if it is newer, otherwise download it.
    
        Only valid for one project.
    '''
    if len(context.project_paths) != 1:
        raise IceError('--sync-utils is only valid for one project!')
    project_path = context.project_paths[0]
    try:
        project_utils = pjoin(project_path, 'utils')
        project_bk = pjoin(project_path, '.utils.bk')
        global_new = not dirs_equal(UTILS_DIR, project_bk)
        project_new = not dirs_equal(project_utils, project_bk)
        if global_new and project_new:
            raise IceError('Both the global utils and the project utils have been modified.')
        elif global_new:
            download_utils(context)
        elif project_new:
            upload_utils(context)
        else:
            print('ice: INFO: Neither the global nor project utils have been modified.')
    except IceError as e:
        print(e, file=sys.stderr)


refresh_lock = threading.Lock()

def refresh(context):
    if (context.args.daemon):
        refresh_daemon(context)
        return
    refresh_lock.acquire()
    try:
        context2 = copy.deepcopy(context)
        context2.project_paths = []
        for project_path in context.project_paths:
            try:
                check_frozen(project_path)
                context2.project_paths.append(project_path)
            except IceError as e:
                print(e, file=sys.stderr)
        all_arg = context2.args.all
        makefile = context2.args.makefile or all_arg
        ycm = context2.args.ycm_extra_conf or all_arg
        utils = context2.args.utils or all_arg
        syncu = context2.args.sync_utils
        pushu = context2.args.push_utils
        if utils:
            if syncu:
                sync_utils(context2)
            elif pushu:
                upload_utils(context2)
            else:
                download_utils(context2)
        if ycm:
            refresh_ycm(context2)
        if makefile:
            refresh_makefile(context2)
    finally:
        refresh_lock.release()

def refresh_daemon(context):
    from watchdog.observers import Observer
    from watchdog.events import FileSystemEventHandler
    context2 = copy.deepcopy(context)
    context2.args.daemon = False
    def start_observer():
        globals()['refresh_observer'] = Observer()
        refresh_observer.schedule(RefreshEventHandler(), UTILS_DIR, recursive=True)
        for project_path in context2.project_paths:
            refresh_observer.schedule(RefreshEventHandler(project_path), project_path, recursive=True)
        refresh_observer.start()
    class RefreshEventHandler (FileSystemEventHandler):
        def __init__(self, project_path=None):
            self.project_path = project_path
        
        def on_any_event(self, event):
            if self.project_path is None:
                context3 = context2
            else:
                context3 = copy.deepcopy(context2)
                context3.project_paths = [self.project_path]
            refresh_observer.stop()
            refresh(context3)
            start_observer()
    start_observer()
    try:
        import time
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        refresh_observer.stop()
    refresh_observer.join()
    print()
            

def refresh_flags(context, all=False, makefile=False, ycm_extra_conf=False,
        utils=False, sync_utils=False, push_utils=False, daemon=False):
    context2 = copy.deepcopy(context)
    context2.args.all = all
    context2.args.makefile = makefile
    context2.args.ycm_extra_conf = ycm_extra_conf
    context2.args.utils = utils
    context2.args.sync_utils = sync_utils
    context2.args.push_utils = push_utils
    context2.args.daemon = daemon
    refresh(context2)


def create(context):
    for project_path in context.project_paths:
        try:
            check_frozen(project_path)
            project_path = realpath(project_path)
            os.makedirs(project_path, exist_ok=True)
            iced_profile = pjoin(project_path, '.iced-profile')
            print(iced_profile)
            with open(iced_profile, 'a') as _:
                pass
            target = context.target
            if target is None:
                target = 'main'
            main_path = pjoin(project_path, '{}.c'.format(target))
            print(main_path)
            shutil.copy2(MAIN_C, main_path)
            refresh_flags(context, all=True)
        except IceError as e:
            print(e, file=sys.stderr)


def make_flags(context, make_args):
    for project_path in context.project_paths:
        project_path = realpath(project_path)
        old_cwd = realpath(os.getcwd())
        os.chdir(project_path)
        import subprocess
        make_args2 = ['make']
        make_args2.extend(make_args)
        try:
            output = subprocess.check_output(make_args2, stderr=subprocess.STDOUT)
            print(output.decode('UTF-8'))
        except subprocess.CalledProcessError as e:
            warn('Make returned exit code {}!'.format(e.returncode))
            print(e.output.decode('UTF-8'))
        os.chdir(old_cwd)

def make(context):
    make_flags(context, context.args.make_args)


def build(context):
    try:
        if context.args.refresh:
            syncu = context.args.sync_utils
            refresh_flags(context, all=True, sync_utils=syncu)
        make_flags(context, ['clean'])
        make_flags(context, ['all'])
        if context.args.program:
            if len(context.project_paths) != 1:
                raise IceError('--program is only valid for one project!')
            make_flags(context, ['program'])
    except IceError as e:
        print(e, file=sys.stderr)


def freeze(context):
    for project_path in context.project_paths:
        frozen_file = pjoin(project_path, '.FROZEN')
        print(frozen_file)
        with open(frozen_file, 'a') as _:
            pass

def thaw(context):
    for project_path in context.project_paths:
        frozen_file = pjoin(project_path, '.FROZEN')
        print(frozen_file)
        try:
            os.remove(frozen_file)
        except FileNotFoundError:
            pass


def main():
    # Initialize parser
    parser = argparse.ArgumentParser(description='Manage projects.', fromfile_prefix_chars='@')
    # Add arguments
    parser.add_argument('-j', '--project', nargs=1, action='append',
            help='the project directories to work with (default: working directory)')
    parser.add_argument('-J', '--projects', nargs='+', action='append', dest='project',
            help='the project directories to work with (default: working directory)')
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
            help='the port to use for avrdude (default: %(default)s). Nullifies --dynamic-port')
    parser.add_argument('-P', '--dynamic-port', action='store_const', const=None, dest='port',
            help='determine the avrdude port dynamically. Nullifies --port')
    parser.add_argument('-t', '--target', default='main',
            help='the name (without the extension) of the file with the main function (default: %(default)s).'
                + ' Nullifies --dynamic-target.')
    parser.add_argument('-T', '--dynamic-target', action='store_const', const=None, dest='target',
            help='dynamically determine the file with the main function. Nullifies --target.')
    parser.add_argument('-S', '--source', '--src', nargs='+', action='append',
            default=[['./*.c', './utils/*.c']],
            help="add sources for the project (relative to the project root) (auto: './*.c' './utils/*.c')")
    parser.add_argument('--no-default-sources', '--nds', action='store_true',
            help="prevent the automatic inclusion of sources './*.c' and './utils/*.c'")
    parser.add_argument('-I', '--include', nargs='+', action='append',
            default=[['./', './utils/']],
            help="add include directories for the project (relative to the project root) (auto: './' './utils/')")
    parser.add_argument('--no-default-includes', '--ndi', action='store_true',
            help="prevent the automatic inclusion of './' and './utils/'")
    
    # Add Subcommands
    def _add_basic_subparsers(_parser):
        '''Add the basic subcommands to a parser.'''
        # Set up subparsers
        _subparsers = _parser.add_subparsers(dest='subcommand', metavar='subcommand')
        # Add subcommands
        parser_create = _subparsers.add_parser('create', help='create project(s)',
                description='Create project(s) in the given directory(s).')
        parser_refresh = _subparsers.add_parser('refresh', help='refresh the project(s)',
                description='Refresh certain files for the project(s).')
        parser_refresh.add_argument('-m', '--makefile', action='store_true',
                help='refresh the makefile for the project(s)')
        parser_refresh.add_argument('-y', '--ycm-extra-conf', action='store_true',
                help='refresh the YCM configuration for the project(s)')
        parser_refresh.add_argument('-u', '--utils', action='store_true',
                help='refresh (download) the utilities folder for the project(s)')
        parser_refresh.add_argument('-s', '--sync-utils', action='store_true',
                help='upload the project utilities folder if it is newer,'
                    + ' otherwise download it. Only valid for one project.')
        parser_refresh.add_argument('-p', '--push-utils', action='store_true',
                help='upload the project utilities folder. Only valid for one project.')
        parser_refresh.add_argument('-a', '--all', action='store_true',
                help='refresh all refreshable resources')
        parser_refresh.add_argument('-d', '--daemon', action='store_true',
                help='run a daemon that will automatically refresh the project(s).')
        parser_build = _subparsers.add_parser('build', help='build the project(s)',
                description='Compiles the project(s) using make.')
        parser_build.add_argument('-r', '--refresh', action='store_true',
                help='refresh before building')
        parser_build.add_argument('-s', '--sync-utils', action='store_true',
                help='sync the utilities folder when refreshing. Only valid for one project.')
        parser_build.add_argument('-p', '--program', action='store_true',
                help='program the microcontroller after compiling. Only valid for one project.')
        parser_freeze = _subparsers.add_parser('freeze', help='freeze the project(s)',
                description='"Freeze" projects: prevent refreshing.')
        parser_thaw = _subparsers.add_parser('thaw', help='unfreeze the project(s)',
                description='"Thaw" projects: re-enable refreshing.')
        parser_make = _subparsers.add_parser('make', help='make the project(s)',
                description='Runs make in the project(s)')
        parser_make.add_argument('make_args', nargs='*',
                help='arguments for make (e.g. clean, all...)')
        return _subparsers

    # Add subcommands to main parser
    subparsers = _add_basic_subparsers(parser)
    # Handle -- with slightly more grace than the default
    parser_ddash = subparsers.add_parser('--', help='end flags',
            description='Terminates the flags section of the command.')
    _add_basic_subparsers(parser_ddash)

    # Parse the arguments
    args = parser.parse_args()
    # Print the parsed arguments
    for (param, arg) in vars(args).items():
        print('{!s}={!r}'.format(param, arg))
    print()
    # Initialize the context
    context = Context()

    # Handle Arguments
    subcommand = context.subcommand = args.subcommand
    # Consolidate and make absolute the project directory list
    if args.project:
        context.project_paths = [realpath(path) for path in chainfi(args.project)]
    else:
        context.project_paths = [realpath(os.curdir)]
    
    context.port_override = args.port
    context.target = args.target
    # Process the source and include lists
    args_source = args.source[:]
    args_include = args.include[:]
    if args.no_default_sources:
        del args_source[0]
    if args.no_default_includes:
        del args_include[0]
    context.sources = list(chainfi(args_source))
    context.includes = list(chainfi(args_include))
    context.args = args

    # Handle Subcommands
    try:
        if subcommand == 'create':
            create(context)
        elif subcommand == 'refresh':
            refresh(context)
        elif subcommand == 'build':
            build(context)
        elif subcommand == 'freeze':
            freeze(context)
        elif subcommand == 'thaw':
            thaw(context)
        elif subcommand == 'make':
            make(context)
        else:
            parser.print_usage()
            print()
            print('Run ice --help for more info.')
    except IceError as e:
        print(e, file=sys.stderr)


if __name__ == '__main__':
    # Run the script
    main()
