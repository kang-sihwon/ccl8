"""Summarize an engine CSV capture without treating Slate ticks as game frames."""
import argparse
import csv
import json
from pathlib import Path


def summarize(path):
    with path.open(encoding='utf-8-sig', newline='') as stream:
        rows=list(csv.DictReader(stream))
    metrics={}
    for key in ('FrameTime','GameThreadTime','RenderThreadTime','GPUTime',
                'RHI/DrawCalls','RHI/PrimitivesDrawn','GPUMem/LocalUsedMB',
                'TextureStreaming/DesiredDataLoadedPercent'):
        values=[]
        for row in rows:
            try:
                if float(row.get('FrameTime',''))>0:
                    values.append(float(row[key]))
            except (KeyError,TypeError,ValueError):
                continue
        values.sort()
        if not values:
            continue
        mean=sum(values)/len(values)
        result={'samples':len(values),'mean':mean,'p50':values[len(values)//2],
                'p95':values[min(len(values)-1,int(len(values)*.95))],
                'p99':values[min(len(values)-1,int(len(values)*.99))],
                'max':values[-1]}
        if key=='FrameTime':
            result.update({'equivalent_mean_fps':1000/mean,
                           'over_16_667ms':sum(x>1000/60 for x in values),
                           'over_33_333ms':sum(x>1000/30 for x in values),
                           'over_50ms':sum(x>50 for x in values)})
        metrics[key]=result
    assert 'FrameTime' in metrics, 'No valid engine frame samples.'
    return {'source':path.name,'metrics':metrics,
            'limits':'All positive FrameTime rows, including profiler/tool overhead. Editor PIE, not packaged performance.'}


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv',type=Path)
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    result=json.dumps(summarize(args.csv),indent=2)
    if args.output:
        args.output.write_text(result,encoding='utf-8')
    print(result)
