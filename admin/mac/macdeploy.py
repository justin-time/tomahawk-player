#!/usr/bin/python
#  This file is part of Tomahawk.
#  It was inspired in large part by the macdeploy script in Clementine.
#
#  Clementine is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Clementine is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Clementine.  If not, see <http://www.gnu.org/licenses/>.

import os
import re
import subprocess
import commands
import sys

FRAMEWORK_SEARCH_PATH=[
    '/Library/Frameworks',
    os.path.join(os.environ['HOME'], 'Library/Frameworks')
]

LIBRARY_SEARCH_PATH=['/usr/local/lib', '/usr/local/Cellar/gettext/0.19.2/lib', '.']

VLC_PLUGINS=[
  'access/libattachment_plugin.dylib',
  #'access/libaccess_avio_plugin.dylib',
  #'access/libaccess_fake_plugin.dylib',
  'access/libftp_plugin.dylib',
  'access/libhttp_plugin.dylib',
  'access/libimem_plugin.dylib',
  #'access/libaccess_mmap_plugin.dylib',
  'access/libaccess_mms_plugin.dylib',
  'access/libaccess_realrtsp_plugin.dylib',
  'access/libtcp_plugin.dylib',
  'access/libudp_plugin.dylib',
  'access/libcdda_plugin.dylib',
  'access/libfilesystem_plugin.dylib',
  'access/libqtcapture_plugin.dylib',
  'access/librtp_plugin.dylib',
  'access/libzip_plugin.dylib',
  'access_output/libaccess_output_dummy_plugin.dylib',
  'access_output/libaccess_output_file_plugin.dylib',
  'access_output/libaccess_output_http_plugin.dylib',
  'access_output/libaccess_output_shout_plugin.dylib',
  'access_output/libaccess_output_udp_plugin.dylib',
  'audio_filter/liba52tofloat32_plugin.dylib',
  'audio_filter/liba52tospdif_plugin.dylib',
  'audio_filter/libaudio_format_plugin.dylib',
  'audio_filter/libaudiobargraph_a_plugin.dylib',
  'audio_filter/libchorus_flanger_plugin.dylib',
  #'libconverter_fixed_plugin.dylib',
  'audio_filter/libdolby_surround_decoder_plugin.dylib',
  'audio_filter/libdtstofloat32_plugin.dylib',
  'audio_filter/libdtstospdif_plugin.dylib',
  'audio_filter/libequalizer_plugin.dylib',
  'audio_filter/libheadphone_channel_mixer_plugin.dylib',
  'audio_filter/libmono_plugin.dylib',
  'audio_filter/libmpgatofixed32_plugin.dylib',
  'audio_filter/libnormvol_plugin.dylib',
  'audio_filter/libparam_eq_plugin.dylib',
  'audio_filter/libscaletempo_plugin.dylib',
  'audio_filter/libsimple_channel_mixer_plugin.dylib',
  'audio_filter/libspatializer_plugin.dylib',
  'audio_filter/libtrivial_channel_mixer_plugin.dylib',
  'audio_filter/libugly_resampler_plugin.dylib',
  'audio_mixer/libfloat_mixer_plugin.dylib',
  #'libspdif_mixer_plugin.dylib',
  #'libtrivial_mixer_plugin.dylib',
  #'libaout_file_plugin.dylib',
  'audio_output/libauhal_plugin.dylib',
  'codec/liba52_plugin.dylib',
  'codec/libadpcm_plugin.dylib',
  'codec/libaes3_plugin.dylib',
  'codec/libaraw_plugin.dylib',
  'codec/libavcodec_plugin.dylib',
  'codec/libcc_plugin.dylib',
  'codec/libcdg_plugin.dylib',
  'codec/libdts_plugin.dylib',
  'codec/libfaad_plugin.dylib',
  #'libfake_plugin.dylib',
  'codec/libflac_plugin.dylib',
  #'libfluidsynth_plugin.dylib',
  #'libinvmem_plugin.dylib',
  'codec/liblpcm_plugin.dylib',
  'codec/libmpeg_audio_plugin.dylib',
  'codec/libpng_plugin.dylib',
  'codec/librawvideo_plugin.dylib',
  'codec/libspeex_plugin.dylib',
  'codec/libspudec_plugin.dylib',
  'codec/libtheora_plugin.dylib',
  'codec/libtwolame_plugin.dylib',
  'codec/libvorbis_plugin.dylib',
  #'control/libgestures_plugin.dylib',
  #'libhotkeys_plugin.dylib',
  #'libmotion_plugin.dylib',
  #'libnetsync_plugin.dylib',
  #'libsignals_plugin.dylib',
  'demux/libaiff_plugin.dylib',
  'demux/libasf_plugin.dylib',
  'demux/libau_plugin.dylib',
  #'libavformat_plugin.dylib',
  'demux/libavi_plugin.dylib',
  'demux/libdemux_cdg_plugin.dylib',
  'demux/libdemuxdump_plugin.dylib',
  'demux/libdiracsys_plugin.dylib',
  'demux/libes_plugin.dylib',
  'demux/libflacsys_plugin.dylib',
  'access/liblive555_plugin.dylib',
  'demux/libmkv_plugin.dylib',
  'demux/libmod_plugin.dylib',
  'demux/libmp4_plugin.dylib',
  'demux/libmpc_plugin.dylib',
  'demux/libmpgv_plugin.dylib',
  'demux/libnsc_plugin.dylib',
  'demux/libnsv_plugin.dylib',
  'demux/libnuv_plugin.dylib',
  'demux/libogg_plugin.dylib',
  'demux/libplaylist_plugin.dylib',
  'demux/libps_plugin.dylib',
  'demux/libpva_plugin.dylib',
  'demux/librawaud_plugin.dylib',
  'demux/librawdv_plugin.dylib',
  'demux/librawvid_plugin.dylib',
  'demux/libreal_plugin.dylib',
  'demux/libsmf_plugin.dylib',
  'demux/libts_plugin.dylib',
  'demux/libtta_plugin.dylib',
  'demux/libty_plugin.dylib',
  'demux/libvc1_plugin.dylib',
  'demux/libvoc_plugin.dylib',
  'demux/libwav_plugin.dylib',
  'demux/libxa_plugin.dylib',
  'meta_engine/libfolder_plugin.dylib',
  'meta_engine/libtaglib_plugin.dylib',
  #'libaudioscrobbler_plugin.dylib',
  'control/libdummy_plugin.dylib',
  'misc/libexport_plugin.dylib',
  #'libfreetype_plugin.dylib',
  #'libgnutls_plugin.dylib',
  'misc/liblogger_plugin.dylib',
  'lua/liblua_plugin.dylib',
  #'libosd_parser_plugin.dylib',
  #'libquartztext_plugin.dylib',
  #'libstats_plugin.dylib',
  'misc/libvod_rtsp_plugin.dylib',
  'misc/libxml_plugin.dylib',
  #'libxtag_plugin.dylib',
  'video_chroma/libi420_rgb_mmx_plugin.dylib',
  'video_chroma/libi420_yuy2_mmx_plugin.dylib',
  'video_chroma/libi422_yuy2_mmx_plugin.dylib',
  #'libmemcpymmx_plugin.dylib',
  #'libmemcpymmxext_plugin.dylib',
  'mux/libmux_asf_plugin.dylib',
  'mux/libmux_avi_plugin.dylib',
  'mux/libmux_dummy_plugin.dylib',
  'mux/libmux_mp4_plugin.dylib',
  'mux/libmux_mpjpeg_plugin.dylib',
  'mux/libmux_ogg_plugin.dylib',
  'mux/libmux_ps_plugin.dylib',
  'mux/libmux_ts_plugin.dylib',
  'mux/libmux_wav_plugin.dylib',
  'packetizer/libpacketizer_copy_plugin.dylib',
  'packetizer/libpacketizer_dirac_plugin.dylib',
  'packetizer/libpacketizer_flac_plugin.dylib',
  'packetizer/libpacketizer_h264_plugin.dylib',
  'packetizer/libpacketizer_mlp_plugin.dylib',
  'packetizer/libpacketizer_mpeg4audio_plugin.dylib',
  'packetizer/libpacketizer_mpeg4video_plugin.dylib',
  'packetizer/libpacketizer_mpegvideo_plugin.dylib',
  'packetizer/libpacketizer_vc1_plugin.dylib',
  'video_chroma/libi420_rgb_sse2_plugin.dylib',
  'video_chroma/libi420_yuy2_sse2_plugin.dylib',
  'video_chroma/libi422_yuy2_sse2_plugin.dylib',
  'stream_filter/libdecomp_plugin.dylib',
  #'access/libstream_filter_rar_plugin.dylib',
  'stream_filter/librecord_plugin.dylib',
  #'libvisual_plugin.dylib',
]

VLC_SEARCH_PATH=[
    '/usr/local/lib/vlc/plugins/',
]

QT_PLUGINS = [
    'crypto/libqca-ossl.dylib',
    'phonon_backend/phonon_vlc.so',
    'sqldrivers/libqsqlite.dylib',
    'imageformats/libqgif.dylib',
    'imageformats/libqico.dylib',
    'imageformats/libqjpeg.dylib',
    'imageformats/libqsvg.dylib',
    'imageformats/libqmng.dylib',
]

SNORE_PLUGINS = [
    'libsnore_backend_growl.so',
    'libsnore_backend_osxnotificationcenter.so',
]

TOMAHAWK_PLUGINS = [
  'libtomahawk_account_xmpp.dylib',
  'libtomahawk_account_google.so',
  'libtomahawk_account_zeroconf.so',
  'libtomahawk_account_hatchet.so',
  'libtomahawk_infoplugin_adium.so',
  'libtomahawk_infoplugin_charts.so',
  'libtomahawk_infoplugin_discogs.so',
  'libtomahawk_infoplugin_echonest.so',
  'libtomahawk_infoplugin_hypem.so',
  'libtomahawk_infoplugin_musicbrainz.so',
  'libtomahawk_infoplugin_musixmatch.so',
  'libtomahawk_infoplugin_newreleases.so',
  'libtomahawk_infoplugin_rovi.so',
  'libtomahawk_infoplugin_snorenotify.so',
  'libtomahawk_infoplugin_spotify.so',
  'libtomahawk_viewpage_dashboard.so',
#  'libtomahawk_viewpage_networkactivity.so',
  'libtomahawk_viewpage_charts.so',
  'libtomahawk_viewpage_newreleases.so',
  'libtomahawk_viewpage_whatsnew_0_8.so',
]

QT_PLUGINS_SEARCH_PATH=[
    '/usr/local/Cellar/qt/HEAD/plugins',
]

SNORE_PLUGINS_SEARCH_PATH=[
    '/usr/local/Cellar/snorenotify/0.5.2/lib/libsnore',
]

class Error(Exception):
  pass


class CouldNotFindQtPluginErrorFindFrameworkError(Error):
  pass


class InstallNameToolError(Error):
  pass


class CouldNotFindQtPluginError(Error):
  pass


class CouldNotFindSnorePluginError(Error):
  pass


class CouldNotFindVLCPluginError(Error):
  pass


class CouldNotFindScriptPluginError(Error):
  pass


if len(sys.argv) < 2:
  print 'Usage: %s <bundle.app>' % sys.argv[0]

bundle_dir = sys.argv[1]
bundle_name = os.path.basename(bundle_dir).split('.')[0]

commands = []

binary_dir = os.path.join(bundle_dir, 'Contents', 'MacOS')
frameworks_dir = os.path.join(bundle_dir, 'Contents', 'Frameworks')
commands.append(['mkdir', '-p', frameworks_dir])
vlcplugins_dir = os.path.join(frameworks_dir, 'vlc', 'plugins')
commands.append(['mkdir', '-p', vlcplugins_dir])
snoreplugins_dir = os.path.join(binary_dir, 'libsnore')
commands.append(['mkdir', '-p', snoreplugins_dir])
resources_dir = os.path.join(bundle_dir, 'Contents', 'Resources')
commands.append(['mkdir', '-p', resources_dir])
plugins_dir = os.path.join(bundle_dir, 'Contents', 'qt-plugins')
binary = os.path.join(bundle_dir, 'Contents', 'MacOS', bundle_name)

fixed_libraries = []
fixed_frameworks = []

def GetBrokenLibraries(binary):
  #print "Checking libs for binary: %s" % binary
  output = subprocess.Popen(['otool', '-L', binary], stdout=subprocess.PIPE).communicate()[0]
  broken_libs = {
      'frameworks': [],
      'libs': []}
  for line in [x.split(' ')[0].lstrip() for x in output.split('\n')[1:]]:
    #print "Checking line: %s" % line
    if not line:  # skip empty lines
      continue
    if os.path.basename(binary) == os.path.basename(line):
      #print "mnope %s-%s" % (os.path.basename(binary), os.path.basename(line))
      continue
    if re.match(r'^\s*/System/', line):
      continue  # System framework
    elif re.match(r'^\s*/usr/lib/', line):
      #print "unix style system lib"
      continue  # unix style system library
    elif re.match(r'Breakpad', line):
      continue  # Manually added by cmake.
    elif re.match(r'^\s*@executable_path', line) or re.match(r'^\s*@loader_path', line):
      # Potentially already fixed library
      if '.framework' in line:
        relative_path = os.path.join(*line.split('/')[3:])
        if not os.path.exists(os.path.join(frameworks_dir, relative_path)):
          broken_libs['frameworks'].append(relative_path)
      else:
        relative_path = os.path.join(*line.split('/')[1:])
        #print "RELPATH %s %s" % (relative_path, os.path.join(binary_dir, relative_path))
        if not os.path.exists(os.path.join(binary_dir, relative_path)):
          broken_libs['libs'].append(relative_path)
    elif re.search(r'\w+\.framework', line):
      broken_libs['frameworks'].append(line)
    else:
      broken_libs['libs'].append(line)

  return broken_libs

def FindFramework(path):
  for search_path in FRAMEWORK_SEARCH_PATH:
    abs_path = os.path.join(search_path, path)
    if os.path.exists(abs_path):
      return abs_path

  raise CouldNotFindFrameworkError(path)

def FindLibrary(path):
  if os.path.exists(path):
    return path
  for search_path in LIBRARY_SEARCH_PATH:
    abs_path = os.path.join(search_path, path)
    if os.path.exists(abs_path):
      return abs_path
    else: # try harder---look for lib name in library folders
      newpath = os.path.join(search_path,os.path.basename(path))
      if os.path.exists(newpath):
        return newpath

  return ""
  #raise CouldNotFindFrameworkError(path)

def FixAllLibraries(broken_libs):
  for framework in broken_libs['frameworks']:
    FixFramework(framework)
  for lib in broken_libs['libs']:
    FixLibrary(lib)

def FixFramework(path):
  if path in fixed_libraries:
    return
  else:
    fixed_libraries.append(path)
  abs_path = FindFramework(path)
  broken_libs = GetBrokenLibraries(abs_path)
  FixAllLibraries(broken_libs)

  new_path = CopyFramework(abs_path)
  id = os.sep.join(new_path.split(os.sep)[3:])
  FixFrameworkId(new_path, id)
  for framework in broken_libs['frameworks']:
    FixFrameworkInstallPath(framework, new_path)
  for library in broken_libs['libs']:
    FixLibraryInstallPath(library, new_path)

def FixLibrary(path):
  if path in fixed_libraries or FindSystemLibrary(os.path.basename(path)) is not None:
    return
  else:
    fixed_libraries.append(path)
  abs_path = FindLibrary(path)
  if abs_path == "":
    print "Could not resolve %s, not fixing!" % path
    return
  broken_libs = GetBrokenLibraries(abs_path)
  FixAllLibraries(broken_libs)

  new_path = CopyLibrary(abs_path)
  FixLibraryId(new_path)
  for framework in broken_libs['frameworks']:
    FixFrameworkInstallPath(framework, new_path)
  for library in broken_libs['libs']:
    FixLibraryInstallPath(library, new_path)

def FixVLCPlugin(abs_path):
  broken_libs = GetBrokenLibraries(abs_path)
  FixAllLibraries(broken_libs)

  #print "Copying plugin....%s %s %s" % (plugins_dir, subdir, os.path.join(abs_path.split('/')[-2:]))
  new_path = os.path.join(vlcplugins_dir, os.path.basename(abs_path))
  args = ['mkdir', '-p', os.path.dirname(new_path)]
  commands.append(args)
  args = ['ditto', '--arch=i386', '--arch=x86_64', abs_path, new_path]
  commands.append(args)
  args = ['chmod', 'u+w', new_path]
  commands.append(args)
  for framework in broken_libs['frameworks']:
    FixFrameworkInstallPath(framework, new_path)
  for library in broken_libs['libs']:
    FixLibraryInstallPath(library, new_path)

def FixPlugin(abs_path, subdir):
  broken_libs = GetBrokenLibraries(abs_path)
  FixAllLibraries(broken_libs)

  new_path = CopyPlugin(abs_path, subdir)
  for framework in broken_libs['frameworks']:
    FixFrameworkInstallPath(framework, new_path)
  for library in broken_libs['libs']:
    FixLibraryInstallPath(library, new_path)

def FixBinary(path):
  broken_libs = GetBrokenLibraries(path)
  FixAllLibraries(broken_libs)
  for framework in broken_libs['frameworks']:
    FixFrameworkInstallPath(framework, path)
  for library in broken_libs['libs']:
    FixLibraryInstallPath(library, path)

def CopyLibrary(path):
  new_path = os.path.join(frameworks_dir, os.path.basename(path))
  args = ['ditto', '--arch=i386', '--arch=x86_64', path, new_path]
  commands.append(args)
  args = ['chmod', 'u+w', new_path]
  commands.append(args)
  return new_path

def CopyPlugin(path, subdir):
  new_path = os.path.join(plugins_dir, subdir, os.path.basename(path))
  args = ['mkdir', '-p', os.path.dirname(new_path)]
  commands.append(args)
  args = ['ditto', '--arch=i386', '--arch=x86_64', path, new_path]
  commands.append(args)
  args = ['chmod', 'u+w', new_path]
  commands.append(args)
  return new_path

def CopyFramework(path):
  parts = path.split(os.sep)
  name = ''
  for i, part in enumerate(parts):
    if re.match(r'\w+\.framework', part):
      name = part[:-10]
      full_path = os.path.join(frameworks_dir, *parts[i:-1])
      break
  args = ['mkdir', '-p', full_path]
  commands.append(args)
  args = ['ditto', '--arch=i386', '--arch=x86_64', path, full_path]
  commands.append(args)
  args = ['chmod', 'u+w', os.path.join(full_path, parts[-1])]
  commands.append(args)

  menu_nib = os.path.join(os.path.split(path)[0], 'Resources', 'qt_menu.nib')
  if os.path.exists(menu_nib):
    args = ['cp', '-rf', menu_nib, resources_dir]
    commands.append(args)

  framework_versions_dir = os.path.join(full_path, '..', '..', 'Versions')
  framework_resources_current_dir = os.path.join(full_path, 'Resources')
  framework_resources_main_dir = os.path.join(full_path, '..', '..', 'Resources')
  framework_current_version = full_path.split(os.sep)[-1]

  # link /Versions/Current to /Versions/$currentVersion
  args = ['ln', '-Fs', framework_current_version, os.path.join(framework_versions_dir, 'Current')]
  commands.append(args)

  # Copy Contents/Info.plist to Resources/Info.plist if Resources/Info.plist does not exist
  # If Contents/Info.plist doesn't exist either, error out. If we actually see this, we can copy QtCore's Info.plist
  info_plist_in_resources = os.path.join(os.path.split(path)[0], '..', '..', 'Resources', 'Info.plist')
  if not os.path.exists(info_plist_in_resources):
    info_plist_in_contents = os.path.join(os.path.split(path)[0], '..', '..', 'Contents', 'Info.plist')
    args = ['mkdir', '-p', framework_resources_current_dir]
    commands.append(args)
    if os.path.exists(info_plist_in_contents):
      args = ['cp', '-rf', info_plist_in_contents, framework_resources_current_dir]
      commands.append(args)
    else:
      print "%s: Framework does not contain an Info.plist file in Contents/ or Resources/ folder." % (path)
      sys.exit(-1)

  # link /Resources to /Versions/Current/Resources
  args = ['ln', '-Fs', 'Versions/Current/Resources', framework_resources_main_dir]
  commands.append(args)

  # link /$name to /Versions/Current/$name
  args = ['ln', '-Fs', os.path.join('Versions/Current/', name), os.path.join(full_path, '..', '..', name)]
  commands.append(args)

  # HACK: CopyFramework is called repeatedly for the same frameworks, but we can't check for the existence of the link from python
  # as the commands are only executed in the end, that's why we remove wrong symlinks afterwards
  args = ['rm', '-rf', os.path.join(framework_resources_main_dir, 'Resources')]
  commands.append(args)
  args = ['rm', '-rf', os.path.join(framework_versions_dir, 'Current', framework_current_version)]
  commands.append(args)

  return os.path.join(full_path, parts[-1])

def FixId(path, library_name):
  id = '@executable_path/../Frameworks/%s' % library_name
  args = ['install_name_tool', '-id', id, path]
  commands.append(args)

def FixLibraryId(path):
  library_name = os.path.basename(path)
  FixId(path, library_name)

def FixFrameworkId(path, id):
  FixId(path, id)

def FixInstallPath(library_path, library, new_path):
  args = ['install_name_tool', '-change', library_path, new_path, library]
  commands.append(args)

def FindSystemLibrary(library_name):
  for path in ['/lib', '/usr/lib']:
    full_path = os.path.join(path, library_name)
    if os.path.exists(full_path):
      return full_path
  return None

def FixLibraryInstallPath(library_path, library):
  system_library = FindSystemLibrary(os.path.basename(library_path))
  if system_library is None:
    new_path = '@executable_path/../Frameworks/%s' % os.path.basename(library_path)
    FixInstallPath(library_path, library, new_path)
  else:
    FixInstallPath(library_path, library, system_library)

def FixFrameworkInstallPath(library_path, library):
  parts = library_path.split(os.sep)
  for i, part in enumerate(parts):
    if re.match(r'\w+\.framework', part):
      full_path = os.path.join(*parts[i:])
      break
  new_path = '@executable_path/../Frameworks/%s' % full_path
  FixInstallPath(library_path, library, new_path)

def FindQtPlugin(name):
  for path in QT_PLUGINS_SEARCH_PATH:
    if os.path.exists(path):
      if os.path.exists(os.path.join(path, name)):
        return os.path.join(path, name)
  raise CouldNotFindQtPluginError(name)

def FindSnorePlugin(name):
  for path in SNORE_PLUGINS_SEARCH_PATH:
    if os.path.exists(path):
      if os.path.exists(os.path.join(path, name)):
        return os.path.join(path, name)
  raise CouldNotFindSnorePluginError(name)

def FindVLCPlugin(name):
  for path in VLC_SEARCH_PATH:
    if os.path.exists(path):
      if os.path.exists(os.path.join(path, name)):
        return os.path.join(path, name)
  raise CouldNotFindVLCPluginError(name)

FixBinary(binary)

for plugin in VLC_PLUGINS:
  FixVLCPlugin(FindVLCPlugin(plugin))

for plugin in TOMAHAWK_PLUGINS:
  FixPlugin(plugin, '../MacOS')

for plugin in SNORE_PLUGINS:
  FixPlugin(FindSnorePlugin(plugin), '../MacOS/libsnore')

try:
  FixPlugin('tomahawk_crash_reporter', '../MacOS')
except:
  print 'Failed to find tomahawk_crash_reporter'

for plugin in QT_PLUGINS:
  FixPlugin(FindQtPlugin(plugin), os.path.dirname(plugin))

if len(sys.argv) <= 2:
  print 'Would run %d commands:' % len(commands)
  for command in commands:
    print ' '.join(command)

  print 'OK?'
  raw_input()

for command in commands:
  p = subprocess.Popen(command)
  os.waitpid(p.pid, 0)
