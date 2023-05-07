import {
  trigger,
  transition,
  query,
  style,
  group,
  animate,
} from '@angular/animations';

export const slider = trigger('routeAnimations', [
  transition('dashboard => schedules', slideTo('right')),
  transition('schedules => dashboard', slideTo('left')),
  transition('* => settings', slideTo('top')),
  transition('settings => *', slideTo('bottom')),
  transition('* => history', slideTo('bottom')),
  transition('history => *', slideTo('top')),
]);

function slideTo(direction: string) {
  const optional = { optional: true };
  return [
    query(
      ':enter, :leave',
      [
        style({
          position: 'absolute',
          [direction]: 0,
          width: '100%',
          height: '100%',
        }),
      ],
      optional
    ),
    query(':enter', [style({ [direction]: '-100%' })]),
    group([
      query(
        ':leave',
        [animate('600ms ease', style({ [direction]: '100%' }))],
        optional
      ),
      query(':enter', [animate('600ms ease', style({ [direction]: '0%' }))]),
    ]),
  ];
}
