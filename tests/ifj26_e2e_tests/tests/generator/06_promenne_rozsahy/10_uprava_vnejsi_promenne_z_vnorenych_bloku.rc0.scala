import ifj26.*

@main def main(): Unit =
  var cnt = 0
  var i = 0
  while i < 4 do
    if i < 2 then
      cnt = cnt + 10
    else
      var j = 0
      while j < 2 do
        cnt = cnt + 1
        j = j + 1
    i = i + 1
  print(cnt)
  print("\n")
