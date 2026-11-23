#!/usr/bin/env python3
"""Run the committed integration SHA in disposable clones; preserve raw evidence."""
import argparse
import datetime
import difflib
import json
import os
from pathlib import Path
import re
import selectors
import shutil
import signal
import subprocess
import tempfile
import time


def stamp():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def output(cmd, cwd=None):
    return subprocess.check_output(cmd, cwd=cwd, text=True, stderr=subprocess.STDOUT).strip()


def monitor(proc, log, timeout):
    """Read through the next shell prompt, rejecting panic and test failures."""
    data = ''
    deadline = time.monotonic() + timeout
    with selectors.DefaultSelector() as sel:
        sel.register(proc.stdout, selectors.EVENT_READ)
        while time.monotonic() < deadline:
            for key, _ in sel.select(min(1, max(0, deadline - time.monotonic()))):
                chunk = os.read(key.fd, 65536)
                if not chunk:
                    raise RuntimeError('QEMU exited before shell prompt')
                text = chunk.decode('utf-8', 'replace')
                log.write(text)
                log.flush()
                data += text
                if re.search(r'panic:|FAIL|FAILED', data):
                    raise RuntimeError('failure/panic in QEMU output')
                if data.endswith('$ '):
                    return data
    raise TimeoutError('QEMU shell/test timeout')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--sha', default='HEAD')
    ap.add_argument('--output', required=True, type=Path)
    ap.add_argument('--mode', choices=['selftest', 'regression', 'all'], default='all')
    ap.add_argument('--cpus', type=int, nargs='+', choices=[1, 3], default=[1, 3], help='CPU counts for regression; selftest always uses 3')
    ap.add_argument('--toolprefix', default='riscv64-linux-gnu-')
    ap.add_argument('--timeout', type=int, default=900, help='seconds per guest command')
    args = ap.parse_args()
    repo = Path(output(['git', 'rev-parse', '--show-toplevel']))
    sha = output(['git', 'rev-parse', args.sha + '^{commit}'], repo)
    dest = args.output.resolve()
    dest.mkdir(parents=True, exist_ok=False)  # Never overwrite an earlier run.
    env = {'source_sha': sha, 'source_status': output(['git', 'status', '--short'], repo),
           'source_policy': 'Only committed source is cloned; configuration patch saved per cell.',
           'started_utc': stamp(), 'host': output(['uname', '-a']),
           'qemu': output(['qemu-system-riscv64', '--version']),
           'compiler': output([args.toolprefix + 'gcc', '--version']),
           'make': output(['make', '--version']), 'ram': '128 MiB',
           'data': 'fresh mkfs image per cell; cachecheck 64 KiB x 3 x 4 readers; rwtest as committed',
           'invocation': __import__('sys').argv}
    (dest / 'environment.json').write_text(json.dumps(env, indent=2) + '\n')
    shutil.copy2(__file__, dest / 'runner.py')
    cells = []
    for ra in (0, 1):
        if args.mode != 'regression':
            cells.append((ra, 3, 1))
        if args.mode != 'selftest':
            cells.extend((ra, cpu, 0) for cpu in dict.fromkeys(args.cpus))
    results = []
    for ra, cpus, selftest in cells:
        name = f'{sha[:12]}-C{ra}-cpus{cpus}-selftest{selftest}'
        row = {'name': name, 'config': f'C{ra}', 'cpus': cpus, 'selftest': selftest,
               'k01': selftest, 'source_sha': sha, 'started_utc': stamp(), 'checks': {}}
        print('START', name, flush=True)
        with tempfile.TemporaryDirectory(prefix='quang-w05-') as tmp:
            work = Path(tmp) / 'src'
            subprocess.run(['git', 'clone', '--quiet', '--no-hardlinks', str(repo), str(work)], check=True)
            subprocess.run(['git', 'checkout', '--quiet', '--detach', sha], cwd=work, check=True)
            param = work / 'kernel/param.h'
            original = param.read_text()
            modified = original
            for flag, value in [('READ_AHEAD_ENABLED', ra), ('BCACHE_SELFTEST', selftest), ('BCACHE_K01', selftest)]:
                modified, count = re.subn(r'(?m)^#define\s+' + flag + r'\s+\d+', f'#define {flag} {value}', modified)
                if count != 1:
                    raise RuntimeError('missing/duplicate config flag: ' + flag)
            param.write_text(modified)
            (dest / (name + '.patch')).write_text(''.join(difflib.unified_diff(original.splitlines(True), modified.splitlines(True), fromfile='a/kernel/param.h', tofile='b/kernel/param.h')))
            row['build_status'] = output(['git', 'status', '--short'], work)
            proc = None
            with (dest / (name + '.log')).open('w') as log:
                log.write(json.dumps(row, indent=2) + '\n')
                log.flush()
                try:
                    subprocess.run(['make', '-j2', 'kernel/kernel', 'fs.img', 'TOOLPREFIX=' + args.toolprefix], cwd=work, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=180)
                    qcmd = ['qemu-system-riscv64', '-machine', 'virt', '-bios', 'none', '-kernel', 'kernel/kernel', '-m', '128M', '-smp', str(cpus), '-nographic', '-global', 'virtio-mmio.force-legacy=false', '-drive', 'file=fs.img,if=none,format=raw,id=x0', '-device', 'virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0']
                    log.write('\nQEMU command: ' + ' '.join(qcmd) + '\n')
                    proc = subprocess.Popen(qcmd, cwd=work, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, start_new_session=True)
                    boot = monitor(proc, log, args.timeout)
                    row['checks']['B01'] = 'PASS'
                    if selftest:
                        for label, marker in [('K01', 'PASS K01:'), ('K03', 'PASS K03:'), ('selftest', 'BCACHE SELFTEST PASSED'), ('K07', 'PASS K07:')]:
                            if marker not in boot:
                                raise RuntimeError('missing ' + marker)
                            row['checks'][label] = 'PASS'
                    elif 'BCACHE SELFTEST' in boot or 'PASS K01:' in boot:
                        raise RuntimeError('self-test unexpectedly enabled')
                    commands = [('cachecheck', 'CACHECHECK PASSED'), ('rwtest', '=== ALL TESTS PASSED ===')]
                    if not selftest:
                        if cpus == 3:
                            commands.append(('stressfs', ''))
                        commands.append(('usertests', 'ALL TESTS PASSED'))
                    for cmd, marker in commands:
                        log.write('\nHOST command: ' + cmd + '\n')
                        proc.stdin.write((cmd + '\n').encode())
                        proc.stdin.flush()
                        text = monitor(proc, log, args.timeout)
                        if marker and marker not in text:
                            raise RuntimeError('missing success marker for ' + cmd)
                        if cmd == 'stressfs' and len(re.findall(r'(?m)^read\r?$', text)) != 5:
                            raise RuntimeError('stressfs did not finish all readers')
                        row['checks'][cmd] = 'PASS'
                    row['result'] = 'PASS'
                except TimeoutError as exc:
                    row['result'] = 'TIMEOUT'
                    row['error'] = str(exc)
                except (RuntimeError, subprocess.SubprocessError, OSError) as exc:
                    row['result'] = 'FAIL'
                    row['error'] = str(exc)
                finally:
                    if proc is not None and proc.poll() is None:
                        os.killpg(proc.pid, signal.SIGTERM)
                        try:
                            proc.wait(timeout=5)
                        except subprocess.TimeoutExpired:
                            os.killpg(proc.pid, signal.SIGKILL)
                            proc.wait()
                    row['ended_utc'] = stamp()
                    log.write('\nHOST RESULT ' + json.dumps(row) + '\n')
        results.append(row)
        (dest / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
        print(row['result'], name, row.get('error', ''), flush=True)
    lines = ['# Quang — kiểm thử bản tích hợp tuần 5', '', f'Source SHA: `{sha}`.', '',
             'Mỗi lượt dùng bản clone riêng và fs.img mới. Chỉ đổi cờ theo patch đi kèm; không sửa kernel trong repo làm việc.', '',
             '| Cấu hình | CPU | Self-test/K01 | Kết quả | Các kiểm tra đã đạt | Log |', '|---|---:|---:|---|---|---|']
    for row in results:
        lines.append(f"| {row['config']} | {row['cpus']} | {row['selftest']} | {row['result']} | {', '.join(row['checks'])} | [{row['name']}.log]({row['name']}.log) |")
    lines += ['', 'K01 xác nhận đường miss/recheck và ánh xạ; chưa xác nhận delta bộ đếm toàn hệ thống của K01. Phần này thuộc đối chiếu của Trung.',
              'Không dùng các lượt này để kết luận D04/D07/D08/R08 hoặc hiệu năng. stressfs chỉ xác nhận chạy xong và trở lại shell, không kiểm tra nội dung dữ liệu.']
    (dest / 'summary.md').write_text('\n'.join(lines) + '\n')
    return int(any(r['result'] != 'PASS' for r in results))


if __name__ == '__main__':
    raise SystemExit(main())
