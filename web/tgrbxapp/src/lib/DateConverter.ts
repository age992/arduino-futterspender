export const getTimestamp = (date: number) => {
  return new Date(date).toLocaleTimeString([], { timeStyle: 'short' });
};

export const timePickerToUnix = (timestamp: string) => {
  let dateString = new Date(0).toDateString();
  let unix = new Date(dateString + ' ' + timestamp);
  return unix.getTime();
};
