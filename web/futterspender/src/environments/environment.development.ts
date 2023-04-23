import { ScheduleMockService } from 'src/services/schedule/schedule.mock.service';
import { ScheduleService } from 'src/services/schedule/schedule.service';
import { StatusMockService } from 'src/services/status/status.mock.service';
import { StatusService } from 'src/services/status/status.service';

export const environment = {
  production: false,
  providers: [
    { provide: StatusService, useClass: StatusMockService },
    { provide: ScheduleService, useClass: ScheduleMockService },
  ],
};
