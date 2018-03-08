Known bugs:
- Due to how my define parser works, Label: !Define = Value will create error messages. Workaround:
  Label: : !Define = Value. However, Label: Anotherlabel = Value will work.
- else is treated as elseif 1 in all contexts, so attaching two else commmands to the same if
  clause, or putting an elseif after an else, is not rejected (though it is rather dumb to do that).

Deprecated features:
All of these should be avoided; they're only listed here to make sure people don't claim they've
  found any easter eggs. They may start throwing warnings in newer versions of Asar.
- if a = b
- fastrom