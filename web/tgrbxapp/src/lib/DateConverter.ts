export const getTimestamp = (unix: number) => {
  return getLocalDate(unix).toLocaleTimeString([], { timeStyle: 'short' });
};

export const getLocalDate = (unix: number) => {
  const unixDate = new Date(unix);
  const localDate = new Date(unix + getMissingOffsetForLocal(unixDate));
  return localDate;
};

export const getMissingOffsetForLocal = (timestamp: Date) => {
  const timestampOffset = timestamp.getTimezoneOffset();
  const currentLocalOffset = new Date().getTimezoneOffset();
  return (Math.abs(currentLocalOffset) - Math.abs(timestampOffset)) * 60 * 1000;
};

export const timePickerToUnix = (timestamp: string) => {
  let dateString = new Date(0).toDateString();
  let unix = new Date(dateString + ' ' + timestamp);
  return unix.getTime() - getMissingOffsetForLocal(unix);
};
