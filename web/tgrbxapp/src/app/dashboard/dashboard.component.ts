import { Component, OnInit } from '@angular/core';
import { MachineStatus } from 'src/models/MachineStatus';
import { Schedule } from 'src/models/Schedule';
import { ScheduleService } from 'src/services/schedule/schedule.service';
import { StatusService } from 'src/services/status/status.service';
import { EScheduleMode } from 'src/lib/EScheduleMode';
import {
  getTimestamp,
  getTodayUnixSeconds,
  getTomorrowUnixSeconds,
} from 'src/lib/DateConverter';
import { SettingsService } from 'src/services/settings/settings.service';
import { Settings } from 'src/models/Settings';
import { HistoryService } from 'src/services/history/history.service';
import { HistoryData } from 'src/models/WebSocketData';
import { EventType } from 'src/models/Event';
import * as Highcharts from 'highcharts';
import { Scale } from 'src/models/ScaleData';

@Component({
  selector: 'dashboard',
  templateUrl: './dashboard.component.html',
  styleUrls: ['./dashboard.component.scss'],
})
export class DashboardComponent implements OnInit {
  public EScheduleMode: typeof EScheduleMode = EScheduleMode;
  public getTimestamp = getTimestamp;

  public MachineStatus: MachineStatus | null = null;
  public LoadingStatus: boolean = false;
  public Connected: boolean = true;

  public settings: Settings | null = null;
  public FetchingSettings = false;

  public currentSchedule: Schedule | null = null;
  public nextFeedingTime: number | null = null;
  public FetchingSchedule: boolean = false;
  public UpdatingActivity: boolean = false;

  public History: HistoryData = {
    schedules: [],
    events: [],
    scaleData: [],
  };

  Highcharts: typeof Highcharts = Highcharts;
  private readonly SCALE_SERIES_OPACITY = 0.2;
  private readonly CONTAINER_SERIES_INDEX = 0;
  private readonly PLATE_SERIES_INDEX = 1;

  chartOptions: Highcharts.Options = {
    credits: { enabled: false },
    title: {
      text: '',
    },
    legend: { enabled: false },
    xAxis: {
      min: getTodayUnixSeconds(),
      max: getTomorrowUnixSeconds(),
      labels: { formatter: (_) => getTimestamp((_.value as number) * 1000) },
      plotLines: [],
    },
    yAxis: {
      title: { text: 'Gewicht' },
      min: 0,
    },
    chart: {
      zooming: { type: 'x' },
    },
    plotOptions: {
      area: {
        connectNulls: true,
        dataGrouping: { enabled: true, groupPixelWidth: 100 },
      },
    },
    series: [
      {
        name: 'Container',
        data: [],
        type: 'area',
        dataGrouping: { enabled: true, groupPixelWidth: 100 },
        opacity: this.SCALE_SERIES_OPACITY,
        enableMouseTracking: false,
      },
      {
        name: 'Plate',
        data: [],
        type: 'area',
        dataGrouping: { enabled: true, groupPixelWidth: 100 },
        opacity: this.SCALE_SERIES_OPACITY,
        enableMouseTracking: false,
      },
    ],
  };
  protected updateFlag = false;

  constructor(
    public statusService: StatusService,
    private scheduleService: ScheduleService,
    private settingsService: SettingsService,
    private historyService: HistoryService
  ) {}

  ngOnInit(): void {
    this.statusService.MachineStatus.subscribe((m: MachineStatus | null) => {
      this.MachineStatus = m;
    });
    this.statusService.Loading.subscribe((l) => {
      this.LoadingStatus = l;
    });
    this.statusService.Connected.subscribe((c) => {
      this.Connected = c;
    });

    this.settingsService.Settings.subscribe((s) => (this.settings = s));
    this.settingsService.FetchingSettings.subscribe(
      (f) => (this.FetchingSettings = f)
    );

    this.scheduleService.Schedules.subscribe((s) => {
      if (!s || s.length == 0) {
        return;
      }

      for (let schedule of s) {
        if (schedule.Selected) {
          this.currentSchedule = schedule;
          if (schedule.Daytimes) {
            this.updateNextFeed(schedule.Daytimes);
          }
          break;
        }
      }
    });

    this.scheduleService.FetchingSchedules.subscribe((l) => {
      this.FetchingSchedule = l;
    });

    this.historyService.HistoryData.subscribe((h) => {
      if (h.events && h.events.length > 0) {
        this.History.events.push(...h.events);
        debugger;
        const newEventFeed = this.History.events
          .filter((_) => _.Type == EventType.Feed)
          .map((_) => {
            return {
              color: '#FF0000', // Red
              width: 50,
              value: _.CreatedOn, // Position, you'll have to translate this to the values on your x axis
            };
          });
        (this.chartOptions.xAxis as any).plotLines = newEventFeed;
        if (
          h.events.findIndex(
            (e) =>
              e.Type == EventType.Feed ||
              e.Type == EventType.MissedFeed ||
              e.Type == EventType.SkippedFeed
          ) != -1 &&
          !!this.currentSchedule?.Daytimes
        ) {
          this.updateNextFeed(this.currentSchedule.Daytimes);
        }
      }
      if (h.scaleData && h.scaleData.length > 0) {
        //this.History.scaleData.push(...h.scaleData); //TODO
        for (let data of h.scaleData) {
          if (
            this.History.scaleData.findIndex(
              (_) => _.ScaleID == data.ScaleID && _.CreatedOn == data.CreatedOn
            ) == -1
          ) {
            this.History.scaleData.push(data);
          }
        }
        // console.log(this.History.scaleData);
        const newContainerData = this.History.scaleData
          .filter((_) => _.ScaleID == Scale.Container)
          .map((_) => [_.CreatedOn, _.Value]);
        const newPlateData = this.History.scaleData
          .filter((_) => _.ScaleID == Scale.Plate)
          .map((_) => [_.CreatedOn, _.Value]);
        (this.chartOptions.series![this.CONTAINER_SERIES_INDEX] as any).data =
          newContainerData;
        (this.chartOptions.series![this.PLATE_SERIES_INDEX] as any).data =
          newPlateData;
        console.log(this.chartOptions.series![0]);
        console.log(this.chartOptions.series![1]);
        this.updateFlag = true;
      }
      if (h.schedules && h.schedules.length > 0) {
        this.History.schedules.push(...h.schedules);
      }
    });
  }

  updateNextFeed = (daytimes: number[]) => {
    daytimes.sort();
    const now = new Date();
    let daytimeNow = new Date(0);
    daytimeNow.setUTCHours(now.getUTCHours());
    daytimeNow.setUTCMinutes(now.getUTCMinutes());
    daytimeNow.setUTCSeconds(now.getUTCSeconds());
    const daytimeNowSeconds = daytimeNow.getTime() / 1000;
    let index = daytimes.findIndex((d) => d / 1000 > daytimeNowSeconds);
    const nextTime = index > -1 ? daytimes[index] : daytimes[0];
    this.nextFeedingTime = nextTime;
  };

  toggleScheduleActive = (active: boolean) => {
    /*const toggledSchedule: Schedule = {
      ...this.currentSchedule!,
      Active: active,
    };*/
    this.UpdatingActivity = true;
    this.scheduleService
      .toggleScheduleActive(this.currentSchedule!, active)
      .subscribe((s) => {
        this.UpdatingActivity = false;
      });
  };

  protected get containerLoadPercentage() {
    if (!this.MachineStatus) {
      return 0;
    }
    return (this.MachineStatus.ContainerLoad / 1000) * 100;
  }

  protected get plateLoadPercentage() {
    if (!this.MachineStatus) {
      return 0;
    }
    return (this.MachineStatus.PlateLoad / 100) * 100;
    // return this.MachineStatus!.PlateLoad / this.settings!.PlateFilling as number;
  }
}
