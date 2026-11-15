import ifj26.*

@main def main(): Unit =
  var line = readLine()
  var n = strLength(line)
  while n > 0 do
    print(line)
    line = readLine()
    n = strLength(line)
